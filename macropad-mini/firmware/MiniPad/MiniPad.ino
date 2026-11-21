// =====================================================================
//  MiniPad: 5 teclas, encoder e LCD ST7735S colorido
//  Placa: Waveshare RP2040-Zero (serve tambem na Raspberry Pi Pico)
//
//  Arduino IDE: core "Raspberry Pi Pico/RP2040/RP2350" (Earle Philhower)
//    Placa: "Waveshare RP2040 Zero"
//    Ferramentas > USB Stack: "Pico SDK" (padrao)
//  Bibliotecas: Adafruit GFX, Adafruit ST7735 and ST7789, Adafruit NeoPixel
//
//  Os dois nucleos do RP2040 trabalham juntos:
//    nucleo 0 (setup/loop)   -> teclas, encoder e teclado USB
//    nucleo 1 (setup1/loop1) -> animacoes na tela e LED RGB
//  Toda a configuracao fica em config.h.
// =====================================================================
#include <Keyboard.h>
#include "config.h"
#include "events.h"
#include "ui.h"
#include "lcd.h"

#if USE_BOARD_LED
#include <Adafruit_NeoPixel.h>
static Adafruit_NeoPixel boardLed(1, BOARD_LED_PIN, BOARD_LED_ORDER + NEO_KHZ800);
#endif

#if DEBUG_SERIAL
#define LOG(...) Serial.printf(__VA_ARGS__)
#else
#define LOG(...)
#endif

// =====================================================================
//  NUCLEO 0: entradas e USB
// =====================================================================
struct Button {
  uint8_t pin;
  bool pressed;          // estado estavel (sem trepidacao)
  bool lastRaw;
  uint32_t changedAt;
};

static Button keys[NUM_KEYS];
static Button encButton;
static const Action *activeAction[NUM_KEYS];   // acao em uso enquanto a tecla esta apertada
static uint8_t currentLayer = 0;
static bool encUsedForLayer = false;           // girou com o botao apertado?
static bool encClickEaten = false;             // o clique so acordou a tela

// ---- encoder por interrupcao (nunca perde passos) ----
static volatile int32_t encCount = 0;
static volatile uint8_t encState = 0;
static const int8_t ENC_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

void encoderISR() {
  uint8_t s = (digitalRead(ENC_PIN_A) << 1) | digitalRead(ENC_PIN_B);
  encState = ((encState << 2) | s) & 0x0F;
  encCount += ENC_TABLE[encState];
}

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

// ---- botoes ----
void initButton(Button &b, uint8_t pin) {
  b.pin = pin;
  pinMode(pin, INPUT_PULLUP);
  delayMicroseconds(100);
  b.lastRaw = digitalRead(pin) == LOW;
  b.pressed = b.lastRaw;
  b.changedAt = millis();
}

// +1 = acabou de apertar, -1 = acabou de soltar, 0 = nada mudou
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

static void send(EventType t, uint8_t a, int16_t b = 0) {
  Event e = {t, a, b};
  events.push(e);
}

// ---- acoes ----
void setLayer(int layer, int dir) {
  currentLayer = (uint8_t)((layer % (int)NUM_LAYERS + NUM_LAYERS) % NUM_LAYERS);
  LOG("Camada %u: %s\n", currentLayer + 1, LAYERS[currentLayer].name);
  send(EV_LAYER, currentLayer, dir);
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
      if (press) setLayer(currentLayer + 1, 1);
      break;
    case A_NONE:
      break;
  }
}

void tapAction(const Action &a) {
  runAction(a, true);
  delay(8);
  runAction(a, false);
}

static bool screenAsleep() { return WAKE_EATS_KEY && uiSleeping; }

void setup() {
#if DEBUG_SERIAL
  Serial.begin(115200);
#endif
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
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
  LOG("\n=== MiniPad iniciado (%u camadas) ===\n", (unsigned)NUM_LAYERS);
}

void loop() {
  uint32_t now = millis();

  // --- teclas ---
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    int ev = updateButton(keys[i], now);
    if (ev == 1) {
      bool eat = screenAsleep();
      send(EV_KEY_DOWN, i);
      if (eat) { LOG("K%u so acordou a tela\n", i + 1); continue; }
      activeAction[i] = &LAYERS[currentLayer].keys[i];
      LOG("K%u -> %s\n", i + 1, activeAction[i]->label);
      runAction(*activeAction[i], true);
    } else if (ev == -1) {
      // solta a acao que foi apertada, mesmo se a camada mudou no meio
      if (activeAction[i]) runAction(*activeAction[i], false);
      activeAction[i] = nullptr;
      send(EV_KEY_UP, i);
    }
  }

  // --- botao do encoder ---
  int encEv = updateButton(encButton, now);
  if (encEv == 1) {
    encUsedForLayer = false;
    encClickEaten = screenAsleep();
    send(EV_ENC_HOLD, currentLayer, 1);
  } else if (encEv == -1) {
    send(EV_ENC_HOLD, currentLayer, 0);
    if (!encUsedForLayer && !encClickEaten) {
      const Action &a = LAYERS[currentLayer].encClick;
      LOG("Clique -> %s\n", a.label);
      tapAction(a);
      send(EV_ENC_CLICK, currentLayer);
    }
  }

  // --- giro do encoder ---
  int d = readEncoderDetents();
  if (d != 0) {
    if (encButton.pressed) {
      encUsedForLayer = true;          // segurando o botao: troca de camada
      setLayer(currentLayer + d, d > 0 ? 1 : -1);
    } else if (screenAsleep()) {
      send(EV_ENC_TURN, currentLayer, 0);   // so acorda
    } else {
      const Action &a = d > 0 ? LAYERS[currentLayer].encRight : LAYERS[currentLayer].encLeft;
      LOG("Giro %+d -> %s\n", d, a.label);
      for (int n = abs(d); n > 0; n--) tapAction(a);
      send(EV_ENC_TURN, currentLayer, d);
    }
  }
}

// =====================================================================
//  NUCLEO 1: tela e LED
// =====================================================================
static uint32_t lastFrameAt = 0;
static float blSmooth = 0;

void setup1() {
  lcdBegin();
#if USE_BOARD_LED
  boardLed.begin();
  boardLed.setBrightness(BOARD_LED_MAX);
  boardLed.show();
#endif
  ui::begin(millis());
}

void loop1() {
  uint32_t now = millis();
  if (now - lastFrameAt < 1000 / FPS) return;
  lastFrameAt = now;

  Event e;
  while (events.pop(e)) ui::handle(e, now);

  gfx::fb = lcdDrawBuffer();
  if (ui::frame(now)) lcdPush();

  // luz de fundo com transicao suave
  float target = ui::backlight(now);
  blSmooth += (target - blSmooth) * (target > blSmooth ? 0.35f : 0.06f);
  uint8_t bl = (uint8_t)(blSmooth + 0.5f);
  if (bl != blLevel) lcdBacklight(bl);

#if USE_BOARD_LED
  uint8_t r, g, b;
  ui::ledColor(now, r, g, b);
  boardLed.setPixelColor(0, r, g, b);
  boardLed.show();
#endif
}
