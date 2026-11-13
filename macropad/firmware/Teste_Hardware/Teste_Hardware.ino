// =====================================================================
//  TESTE DE HARDWARE do MacroPad (RP2350-Zero)
//
//  Grave este sketch, abra o Monitor Serial (115200) e veja:
//   1. O LED RGB da propria placa (GP16) pisca colorido -> o programa roda
//   2. O Monitor Serial mostra "vivo 1, vivo 2..."    -> USB/serial ok
//   3. O LCD troca de cor a cada segundo               -> LCD ok
//      O teste tenta 3 jeitos de falar com o LCD, 8 s cada, em ciclo.
//      A serial mostra qual tentativa esta rodando: anote em qual aparece imagem.
//   4. Apertar teclas / girar o encoder aparece na serial -> fiacao ok
//
//  Placa: Waveshare RP2350 Zero. USB Stack: Pico SDK.
// =====================================================================
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>

// Mesmos pinos do config.h do MacroPad
#define LCD_SCK   14
#define LCD_MOSI  15
#define LCD_DC    12
#define LCD_BL    13
#define LCD_CS    -1   // CS ligado no GND
#define LCD_RST   -1   // RES ligado no 3V3
#define ENC_A     26
#define ENC_B     27
#define ENC_SW    28
#define ONBOARD_LED 16 // LED RGB que ja vem na RP2350-Zero

static const uint8_t KEY_PINS[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

// Nivel que acende a luz de fundo (HIGH na maioria dos modulos)
#define BL_ON HIGH

Adafruit_ST7789 *tft = nullptr;
int attempt = 0;
uint32_t attemptStart = 0;
const uint8_t NUM_ATTEMPTS = 3;
const char *ATTEMPT_NAMES[NUM_ATTEMPTS] = {
  "SPI1 por hardware, modo 0 (padrao da biblioteca)",
  "SPI1 por hardware, modo 3 (comum quando o CS fica no GND)",
  "SPI por software / bit-bang (ignora o periferico SPI)",
};

void startAttempt(int a) {
  if (tft) {
    delete tft;
    tft = nullptr;
  }
  SPI1.end();
  attempt = a;
  Serial.printf("\n=== Tentativa %d/%d: %s ===\n", a + 1, NUM_ATTEMPTS, ATTEMPT_NAMES[a]);
  if (a < 2) {
    SPI1.setSCK(LCD_SCK);
    SPI1.setTX(LCD_MOSI);
    tft = new Adafruit_ST7789(&SPI1, LCD_CS, LCD_DC, LCD_RST);
    tft->init(172, 320, a == 0 ? SPI_MODE0 : SPI_MODE3);
    tft->setSPISpeed(4000000);   // devagar de proposito, para tolerar fios longos
  } else {
    tft = new Adafruit_ST7789(LCD_CS, LCD_DC, LCD_MOSI, LCD_SCK, LCD_RST);
    tft->init(172, 320);
  }
  tft->setRotation(1);
  attemptStart = millis();
}
Adafruit_NeoPixel onboard(1, ONBOARD_LED, NEO_GRB + NEO_KHZ800);

uint8_t lastState[15];
uint8_t inputPins[15];
const char *inputNames[15] = {
  "K1", "K2", "K3", "K4", "K5", "K6", "K7", "K8", "K9", "K10", "K11", "K12",
  "Encoder A", "Encoder B", "Encoder botao"
};

void setup() {
  onboard.begin();
  onboard.setBrightness(30);
  onboard.setPixelColor(0, onboard.Color(0, 0, 255));   // azul = iniciando
  onboard.show();

  Serial.begin(115200);
  // espera ate 5 s o Monitor Serial abrir, para nao perder as primeiras mensagens
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);

  Serial.println();
  Serial.println("===== TESTE DE HARDWARE DO MACROPAD =====");

  for (uint8_t i = 0; i < 12; i++) inputPins[i] = KEY_PINS[i];
  inputPins[12] = ENC_A;
  inputPins[13] = ENC_B;
  inputPins[14] = ENC_SW;
  for (uint8_t i = 0; i < 15; i++) {
    pinMode(inputPins[i], INPUT_PULLUP);
    lastState[i] = digitalRead(inputPins[i]);
  }
  Serial.println("Entradas configuradas. Teclas soltas devem ler 1 (HIGH):");
  for (uint8_t i = 0; i < 15; i++) {
    Serial.printf("  %-14s GP%-2u = %u%s\n", inputNames[i], inputPins[i], lastState[i],
                  (i < 12 && lastState[i] == 0) ? "   <-- LOW sem apertar? curto com GND?" : "");
  }

  Serial.println("Luz de fundo ligada (BL = GP13).");
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, BL_ON);
  Serial.println("LCD: SCL=GP14, SDA=GP15, DC=GP12, CS=GND, RES=3V3");
  startAttempt(0);
}

void loop() {
  static uint32_t lastTick = 0;
  static uint16_t n = 0;

  // --- a cada 1 s: pisca o LED da placa, troca a cor do LCD, avisa na serial ---
  if (millis() - lastTick >= 1000) {
    lastTick = millis();
    n++;
    static const uint16_t colors[] = {ST77XX_RED, ST77XX_GREEN, ST77XX_BLUE, ST77XX_WHITE};
    static const char *names[] = {"VERMELHO", "VERDE", "AZUL", "BRANCO"};
    uint8_t c = n % 4;

    onboard.setPixelColor(0, n % 2 ? onboard.Color(0, 255, 0) : onboard.Color(255, 0, 255));
    onboard.show();

    if (millis() - attemptStart >= 8000) startAttempt((attempt + 1) % NUM_ATTEMPTS);

    tft->fillScreen(colors[c]);
    tft->setTextColor(c == 3 ? ST77XX_BLACK : ST77XX_WHITE);
    tft->setTextSize(4);
    tft->setCursor(20, 40);
    tft->print(names[c]);
    tft->setTextSize(3);
    tft->setCursor(20, 100);
    tft->printf("tentativa %d", attempt + 1);

    Serial.printf("vivo %u - tentativa %d - LCD deveria estar %s\n", n, attempt + 1, names[c]);
  }

  // --- teclas e encoder ---
  for (uint8_t i = 0; i < 15; i++) {
    uint8_t s = digitalRead(inputPins[i]);
    if (s != lastState[i]) {
      lastState[i] = s;
      Serial.printf("%s (GP%u) %s\n", inputNames[i], inputPins[i], s ? "solto" : "APERTADO / LOW");
    }
  }
}
