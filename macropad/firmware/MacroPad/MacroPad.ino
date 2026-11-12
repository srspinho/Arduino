// =====================================================================
//  MacroPad 4x3 + encoder rotativo  -  RP2350 (Waveshare RP2350-Zero / Pico 2)
//
//  Placa no Arduino IDE: core "Raspberry Pi Pico/RP2040/RP2350"
//  (Earle Philhower). Ferramentas > USB Stack: "Pico SDK" (padrao).
//
//  Bibliotecas: Adafruit NeoPixel, Adafruit GFX, Adafruit ST7735 and ST7789
//  (para o LCD) ou Adafruit SSD1306 (para o OLED).
//  Toda a configuracao fica em config.h.
// =====================================================================
#include <Keyboard.h>
#include "config.h"

#if USE_LEDS
#include <Adafruit_NeoPixel.h>
Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

#include "display.h"

// ---------------------------------------------------------------------
//  Estado
// ---------------------------------------------------------------------
struct Button {
  uint8_t pin;
  bool pressed;          // estado estavel (debounced)
  bool lastRaw;
  uint32_t changedAt;
};

static Button keys[12];
static Button encButton;
static const Action *activeAction[12];   // acao em uso enquanto a tecla esta pressionada

static uint8_t currentLayer = 0;
static bool ledsDirty = true;
static bool asleep = false;
static uint32_t lastActivity = 0;

static bool encUsedForLayer = false;     // girou com o botao pressionado?

// ---------------------------------------------------------------------
//  Encoder (por interrupcao, nunca perde passos)
// ---------------------------------------------------------------------
static volatile int32_t encCount = 0;
static volatile uint8_t encState = 0;
static const int8_t ENC_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void encoderISR() {
  uint8_t s = (digitalRead(ENC_PIN_A) << 1) | digitalRead(ENC_PIN_B);
  encState = ((encState << 2) | s) & 0x0F;
  encCount += ENC_TABLE[encState];
}

// Retorna quantos "cliques" o encoder girou desde a ultima chamada
int readEncoderDetents() {
  noInterrupts();
  int32_t c = encCount;
  int detents = c / ENC_STEPS_PER_DETENT;
  encCount = c - detents * ENC_STEPS_PER_DETENT;
  interrupts();
#if ENC_REVERSE
  detents = -detents;
#endif
  return detents;
}

// ---------------------------------------------------------------------
//  Botoes com debounce
// ---------------------------------------------------------------------
void initButton(Button &b, uint8_t pin) {
  b.pin = pin;
  pinMode(pin, INPUT_PULLUP);
  b.lastRaw = digitalRead(pin) == LOW;
  b.pressed = b.lastRaw;
  b.changedAt = millis();
}

// Retorna +1 se acabou de pressionar, -1 se acabou de soltar, 0 se nada mudou
int updateButton(Button &b, uint32_t now) {
  bool raw = digitalRead(b.pin) == LOW;
  if (raw != b.lastRaw) {
    b.lastRaw = raw;
    b.changedAt = now;
  }
  if (raw != b.pressed && (now - b.changedAt) >= DEBOUNCE_MS) {
    b.pressed = raw;
    return raw ? 1 : -1;
  }
  return 0;
}

// ---------------------------------------------------------------------
//  Acoes
// ---------------------------------------------------------------------
void setLayer(int layer) {
  currentLayer = (uint8_t)((layer % NUM_LAYERS + NUM_LAYERS) % NUM_LAYERS);
  ledsDirty = true;
  if (!asleep) displayLayer(currentLayer);
}

void runAction(const Action &a, bool press) {
  switch (a.type) {
    case A_KEYS:
      for (uint8_t k : a.keys) {
        if (!k) continue;
        if (press) Keyboard.press(k);
        else Keyboard.release(k);
      }
      break;
    case A_MEDIA:
      if (press) Keyboard.consumerPress(a.media);
      else Keyboard.consumerRelease();
      break;
    case A_TEXT:
      if (press) Keyboard.print(a.text);
      break;
    case A_LAYER_NEXT:
      if (press) setLayer(currentLayer + 1);
      return;   // setLayer ja atualizou o rotulo
    case A_NONE:
      return;
  }
  if (press && !asleep) displayStatus(a.label);
}

// Pressiona e solta (usado pelo encoder)
void tapAction(const Action &a) {
  runAction(a, true);
  delay(8);
  runAction(a, false);
}

// ---------------------------------------------------------------------
//  LEDs e tela
// ---------------------------------------------------------------------
bool anyKeyPressed() {
  for (uint8_t i = 0; i < 12; i++)
    if (keys[i].pressed) return true;
  return false;
}

void updateLeds() {
#if USE_LEDS
  if (!ledsDirty) return;
  ledsDirty = false;
  const Layer &L = LAYERS[currentLayer];
  for (uint8_t i = 0; i < LED_COUNT; i++) {
    if (asleep) leds.setPixelColor(i, 0);
#if LED_PER_KEY
    else if (i < 12 && keys[i].pressed) leds.setPixelColor(i, leds.Color(255, 255, 255));
#else
    else if (anyKeyPressed()) leds.setPixelColor(i, leds.Color(255, 255, 255));
#endif
    else leds.setPixelColor(i, leds.Color(L.r, L.g, L.b));
  }
  leds.show();
#endif
}

void wake(uint32_t now) {
  lastActivity = now;
  if (asleep) {
    asleep = false;
    ledsDirty = true;
    displaySleep(false);
    displayLayer(currentLayer);
  }
}

// ---------------------------------------------------------------------
//  setup / loop
// ---------------------------------------------------------------------
void setup() {
  for (uint8_t i = 0; i < 12; i++) {
    initButton(keys[i], KEY_PINS[i]);
    activeAction[i] = nullptr;
  }
  initButton(encButton, ENC_PIN_SW);

  pinMode(ENC_PIN_A, INPUT_PULLUP);
  pinMode(ENC_PIN_B, INPUT_PULLUP);
  encState = (digitalRead(ENC_PIN_A) << 1) | digitalRead(ENC_PIN_B);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_B), encoderISR, CHANGE);

  Keyboard.begin();

#if USE_LEDS
  leds.begin();
  leds.setBrightness(LED_BRIGHTNESS);
  leds.clear();
  leds.show();
#endif

  displayBegin();
  setLayer(0);
  displayStatus("Pronto!");
  lastActivity = millis();
}

void loop() {
  uint32_t now = millis();

  // --- teclas ---
  for (uint8_t i = 0; i < 12; i++) {
    int ev = updateButton(keys[i], now);
    if (ev == 1) {
      wake(now);
      activeAction[i] = &LAYERS[currentLayer].keys[i];
      runAction(*activeAction[i], true);
      displayKey(currentLayer, i, true);
      ledsDirty = true;
    } else if (ev == -1) {
      // solta a acao que foi pressionada, mesmo se a camada mudou no meio
      if (activeAction[i]) runAction(*activeAction[i], false);
      activeAction[i] = nullptr;
      if (!asleep) displayKey(currentLayer, i, false);
      ledsDirty = true;
    }
  }

  // --- botao do encoder ---
  int encEv = updateButton(encButton, now);
  if (encEv == 1) {
    wake(now);
    encUsedForLayer = false;
  } else if (encEv == -1) {
    if (!encUsedForLayer) tapAction(LAYERS[currentLayer].encClick);
  }

  // --- giro do encoder ---
  int d = readEncoderDetents();
  if (d != 0) {
    wake(now);
    if (encButton.pressed) {
      // girar com o botao apertado = trocar de camada
      encUsedForLayer = true;
      setLayer(currentLayer + d);
    } else {
      const Action &a = d > 0 ? LAYERS[currentLayer].encRight : LAYERS[currentLayer].encLeft;
      for (int n = abs(d); n > 0; n--) tapAction(a);
    }
  }

  // --- descanso ---
  if (SLEEP_AFTER_MS > 0 && !asleep && (now - lastActivity) > SLEEP_AFTER_MS) {
    asleep = true;
    ledsDirty = true;
    displaySleep(true);
  }

  updateLeds();
  displayTask();
}
