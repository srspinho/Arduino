// =====================================================================
//  MiniPad: TESTE DE HARDWARE
//  Grave este sketch ANTES do firmware principal para conferir a ligacao.
//
//  O que aparece:
//   - Faixas de cor no topo: VERMELHO, VERDE, AZUL, BRANCO (nesta ordem).
//       * vermelho e azul trocados -> LCD_BGR 1 no config.h do MiniPad
//       * cores em "negativo"      -> LCD_INVERT 1
//       * borda com lixo / imagem deslocada -> troque LCD_INIT
//   - 5 quadrados (teclas): acendem quando a tecla e apertada
//   - Contador do encoder e estado do botao
//   - O LED da placa troca de cor a cada tecla
//  Tudo tambem sai no Monitor Serial (115200).
//
//  Os pinos abaixo devem ser IGUAIS aos do config.h.
// =====================================================================
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_NeoPixel.h>

// ---- mesmos valores do config.h ----
static const uint8_t KEY_PINS[5] = {0, 1, 2, 3, 4};
#define ENC_PIN_A  5
#define ENC_PIN_B  6
#define ENC_PIN_SW 7
#define LCD_SCK  10
#define LCD_MOSI 11
#define LCD_CS   9
#define LCD_DC   8
#define LCD_RST  12
#define LCD_BL   13
#define LCD_INIT INITR_BLACKTAB   // 0.96": INITR_MINI160x80   1.44": INITR_144GREENTAB
#define LCD_ROTATION 1
#define LCD_INVERT 0
#define LED_PIN  16

Adafruit_ST7735 tft(&SPI1, LCD_CS, LCD_DC, LCD_RST);
Adafruit_NeoPixel led(1, LED_PIN, NEO_RGB + NEO_KHZ800);

volatile int32_t encCount = 0;
volatile uint8_t encState = 0;
const int8_t ENC_TABLE[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};
void encoderISR() {
  uint8_t s = (digitalRead(ENC_PIN_A) << 1) | digitalRead(ENC_PIN_B);
  encState = ((encState << 2) | s) & 0x0F;
  encCount += ENC_TABLE[encState];
}

bool lastKey[5], lastSw;
int32_t lastEnc = -999;
uint8_t ledStep = 0;

void drawKey(uint8_t i, bool on) {
  int16_t w = tft.width() / 5;
  int16_t y = tft.height() - 26;
  tft.fillRoundRect(i * w + 2, y, w - 4, 22, 4, on ? ST77XX_YELLOW : 0x2104);
  tft.setTextColor(on ? ST77XX_BLACK : ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(i * w + w / 2 - 6, y + 7);
  tft.print("K");
  tft.print(i + 1);
}

void drawEncoder(int32_t count, bool sw) {
  tft.fillRect(0, 26, tft.width(), 24, ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(4, 30);
  tft.print("Enc ");
  tft.print(count);
  tft.setTextSize(1);
  tft.setCursor(tft.width() - 34, 34);
  tft.setTextColor(sw ? ST77XX_GREEN : 0x8410);
  tft.print(sw ? "BOTAO" : "botao");
}

void setup() {
  Serial.begin(115200);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  SPI1.setSCK(LCD_SCK);
  SPI1.setTX(LCD_MOSI);
  tft.initR(LCD_INIT);
  tft.setSPISpeed(16000000);
  tft.setRotation(LCD_ROTATION);
  tft.invertDisplay(LCD_INVERT);
  tft.fillScreen(ST77XX_BLACK);

  // faixas de cor
  int16_t w = tft.width() / 4;
  tft.fillRect(0, 0, w, 22, ST77XX_RED);
  tft.fillRect(w, 0, w, 22, ST77XX_GREEN);
  tft.fillRect(2 * w, 0, w, 22, ST77XX_BLUE);
  tft.fillRect(3 * w, 0, tft.width() - 3 * w, 22, ST77XX_WHITE);
  // moldura: deve aparecer inteira, 1 px na borda
  tft.drawRect(0, 0, tft.width(), tft.height(), ST77XX_MAGENTA);

  for (uint8_t i = 0; i < 5; i++) {
    pinMode(KEY_PINS[i], INPUT_PULLUP);
    lastKey[i] = false;
    drawKey(i, false);
  }
  pinMode(ENC_PIN_A, INPUT_PULLUP);
  pinMode(ENC_PIN_B, INPUT_PULLUP);
  pinMode(ENC_PIN_SW, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_A), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_PIN_B), encoderISR, CHANGE);

  led.begin();
  led.setBrightness(40);
  led.setPixelColor(0, 255, 0, 0);   // deve acender VERMELHO
  led.show();
  Serial.println("Teste de hardware: LED deve estar VERMELHO. Se estiver verde, use NEO_GRB.");
}

void loop() {
  for (uint8_t i = 0; i < 5; i++) {
    bool on = digitalRead(KEY_PINS[i]) == LOW;
    if (on != lastKey[i]) {
      lastKey[i] = on;
      drawKey(i, on);
      Serial.printf("K%u (GP%u) %s\n", i + 1, KEY_PINS[i], on ? "apertada" : "solta");
      if (on) {
        static const uint8_t cores[4][3] = {{255, 0, 0}, {0, 255, 0}, {0, 0, 255}, {255, 255, 255}};
        ledStep = (ledStep + 1) % 4;
        led.setPixelColor(0, cores[ledStep][0], cores[ledStep][1], cores[ledStep][2]);
        led.show();
      }
    }
  }
  bool sw = digitalRead(ENC_PIN_SW) == LOW;
  int32_t c = encCount / 4;
  if (c != lastEnc || sw != lastSw) {
    lastEnc = c;
    lastSw = sw;
    drawEncoder(c, sw);
    Serial.printf("Encoder %ld  botao %s\n", (long)c, sw ? "apertado" : "solto");
  }
  delay(5);
}
