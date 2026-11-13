// =====================================================================
//  Tela: LCD ST7789 172x320 (colorido) ou OLED SSD1306
//  Funcoes usadas pelo MacroPad.ino:
//    displayBegin()               inicia a tela
//    displayLayer(layer)          desenha tudo (troca de camada / acordar)
//    displayKey(layer, i, down)   redesenha so a tecla i
//    displayStatus(texto)         mostra a ultima acao
//    displaySleep(dormindo)       apaga / acende
//    displayTask()                chamado no loop (OLED desenha aqui)
// =====================================================================
#pragma once
#include "config.h"

static inline bool isLightColor(uint8_t r, uint8_t g, uint8_t b) {
  return (r * 299 + g * 587 + b * 114) / 1000 > 140;
}

#if DISPLAY_TYPE == DISPLAY_LCD
// ---------------------------------------------------------------------
//  LCD colorido ST7789
// ---------------------------------------------------------------------
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

static Adafruit_ST7789 tft(&SPI1, LCD_CS, LCD_DC, LCD_RST);

// Layout em modo deitado: 320 x 172
static const int16_t SCR_W = LCD_HEIGHT, SCR_H = LCD_WIDTH;
static const int16_t HEAD_H = 26;                              // cabecalho
static const int16_t FOOT_H = 22;                              // rodape (encoder)
static const int16_t CELL_W = SCR_W / 4;                       // 80
static const int16_t CELL_H = (SCR_H - HEAD_H - FOOT_H) / 3;   // 41

// Cada parte e desenhada primeiro na memoria e depois enviada de uma vez:
// assim a tela nao pisca.
static GFXcanvas16 headCanvas(SCR_W, HEAD_H);
static GFXcanvas16 footCanvas(SCR_W, FOOT_H);
static GFXcanvas16 cellCanvas(CELL_W, CELL_H);

static const uint16_t C_BG   = 0x0000;   // preto
static const uint16_t C_CELL = 0x18E3;   // cinza escuro
static const uint16_t C_TEXT = 0xFFFF;   // branco
static const uint16_t C_DIM  = 0x9CD3;   // cinza claro

static const GFXfont *F_BOLD = &FreeSansBold9pt7b;
static const GFXfont *F_REG  = &FreeSans9pt7b;

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static uint16_t layerColor(const Layer &L) { return rgb565(L.r, L.g, L.b); }
static uint16_t layerTextColor(const Layer &L) { return isLightColor(L.r, L.g, L.b) ? 0x0000 : 0xFFFF; }

static int16_t textWidth(Adafruit_GFX &g, const char *t, const GFXfont *f) {
  int16_t x1, y1;
  uint16_t w, h;
  g.setTextWrap(false);
  g.setFont(f);
  g.setTextSize(1);
  g.getTextBounds(t, 0, 50, &x1, &y1, &w, &h);
  return w;
}

// Escreve uma linha. align: 0 = centro em x, -1 = comeca em x, 1 = termina em x
static void printLine(Adafruit_GFX &g, const char *t, const GFXfont *f, int16_t x, int16_t baseline, int align = 0) {
  int16_t x1, y1;
  uint16_t w, h;
  g.setTextWrap(false);
  g.setFont(f);
  g.setTextSize(1);
  g.getTextBounds(t, 0, baseline, &x1, &y1, &w, &h);
  int16_t left = align == 0 ? x - w / 2 : (align < 0 ? x : x - w);
  g.setCursor(left - x1, baseline);
  g.print(t);
}

// Rotulo de uma tecla: negrito numa linha -> negrito em 2 linhas -> normal -> fonte pequena
static void drawLabel(Adafruit_GFX &g, const char *txt, int16_t cx, int16_t cy, int16_t maxW) {
  if (textWidth(g, txt, F_BOLD) <= maxW) {
    printLine(g, txt, F_BOLD, cx, cy + 6);
    return;
  }
  // tenta quebrar em 2 linhas no espaco ou depois da barra
  const char *brk = strchr(txt, ' ');
  if (!brk) brk = strchr(txt, '/');
  if (brk) {
    char a[24];
    size_t n = brk - txt + (*brk == '/' ? 1 : 0);
    if (n > sizeof(a) - 1) n = sizeof(a) - 1;
    memcpy(a, txt, n);
    a[n] = 0;
    const char *b = brk + 1;
    if (textWidth(g, a, F_BOLD) <= maxW && textWidth(g, b, F_BOLD) <= maxW) {
      printLine(g, a, F_BOLD, cx, cy - 2);
      printLine(g, b, F_BOLD, cx, cy + 14);
      return;
    }
  }
  if (textWidth(g, txt, F_REG) <= maxW) {
    printLine(g, txt, F_REG, cx, cy + 6);
    return;
  }
  g.setFont(nullptr);   // ultimo recurso: fonte pequena embutida
  g.setTextSize(1);
  g.setCursor(cx - strlen(txt) * 3, cy - 4);
  g.print(txt);
}

static uint8_t lcdLayer = 0;

static void setBacklight(uint8_t level) {
  analogWrite(LCD_BL, LCD_BL_ACTIVE_LOW ? 255 - level : level);
}

void displayKey(uint8_t layer, uint8_t i, bool down) {
  const Layer &L = LAYERS[layer];
  GFXcanvas16 &c = cellCanvas;
  uint16_t accent = layerColor(L);
  c.fillScreen(C_BG);
  c.fillRoundRect(2, 2, CELL_W - 4, CELL_H - 4, 6, down ? accent : C_CELL);
  c.drawRoundRect(2, 2, CELL_W - 4, CELL_H - 4, 6, accent);
  c.setTextColor(down ? layerTextColor(L) : C_TEXT);
  drawLabel(c, L.keys[i].type == A_NONE ? "-" : L.keys[i].label, CELL_W / 2, CELL_H / 2, CELL_W - 9);
  tft.drawRGBBitmap((i % 4) * CELL_W, HEAD_H + (i / 4) * CELL_H, c.getBuffer(), CELL_W, CELL_H);
}

void displayStatus(const char *status) {
  const Layer &L = LAYERS[lcdLayer];
  GFXcanvas16 &c = headCanvas;
  c.fillScreen(layerColor(L));
  c.setTextColor(layerTextColor(L));
  printLine(c, L.name, F_BOLD, 6, 19, -1);
  char buf[32];
  snprintf(buf, sizeof(buf), "%s   %u/%u", status, lcdLayer + 1, NUM_LAYERS);
  printLine(c, buf, F_REG, SCR_W - 6, 19, 1);
  tft.drawRGBBitmap(0, 0, c.getBuffer(), SCR_W, HEAD_H);
}

static void drawFooter(const Layer &L) {
  GFXcanvas16 &c = footCanvas;
  char buf[28];
  c.fillScreen(C_BG);
  c.setTextColor(C_DIM);
  snprintf(buf, sizeof(buf), "< %s", L.encLeft.label);
  printLine(c, buf, F_REG, 4, 16, -1);
  snprintf(buf, sizeof(buf), "%s >", L.encRight.label);
  printLine(c, buf, F_REG, SCR_W - 4, 16, 1);
  c.setTextColor(layerColor(L));
  snprintf(buf, sizeof(buf), "[%s]", L.encClick.label);
  printLine(c, buf, F_BOLD, SCR_W / 2, 16);
  tft.drawRGBBitmap(0, SCR_H - FOOT_H, c.getBuffer(), SCR_W, FOOT_H);
}

void displayBegin() {
  pinMode(LCD_BL, OUTPUT);
  setBacklight(0);
  SPI1.setSCK(LCD_SCK);
  SPI1.setTX(LCD_MOSI);
  tft.init(LCD_WIDTH, LCD_HEIGHT);
  tft.setSPISpeed(LCD_SPI_HZ);
  tft.invertDisplay(LCD_INVERT);
  tft.setRotation(LCD_ROTATION);
  tft.fillScreen(C_BG);
  setBacklight(LCD_BRIGHTNESS);
}

void displayLayer(uint8_t layer) {
  lcdLayer = layer;
  displayStatus("");
  for (uint8_t i = 0; i < 12; i++) displayKey(layer, i, false);
  drawFooter(LAYERS[layer]);
}

void displaySleep(bool sleeping) {
  setBacklight(sleeping ? 0 : LCD_BRIGHTNESS);
  if (sleeping) tft.fillScreen(C_BG);
}

void displayTask() {}

#elif DISPLAY_TYPE == DISPLAY_OLED
// ---------------------------------------------------------------------
//  OLED SSD1306 (monocromatico)
// ---------------------------------------------------------------------
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);
static bool oledOk = false, oledDirty = true, oledSleeping = false;
static uint8_t oledLayer = 0;
static const char *oledStatus = "";

void displayBegin() {
  Wire.setSDA(OLED_SDA);
  Wire.setSCL(OLED_SCL);
  Wire.begin();
  Wire.setClock(400000);
  oledOk = oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
}
void displayLayer(uint8_t layer) { oledLayer = layer; oledStatus = LAYERS[layer].name; oledDirty = true; }
void displayKey(uint8_t, uint8_t, bool) {}
void displayStatus(const char *s) { oledStatus = s; oledDirty = true; }
void displaySleep(bool s) { oledSleeping = s; oledDirty = true; }

void displayTask() {
  if (!oledOk || !oledDirty) return;
  oledDirty = false;
  oled.clearDisplay();
  if (!oledSleeping) {
    oled.setTextColor(SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(0, 0);
    oled.printf("[%u/%u] %s", oledLayer + 1, NUM_LAYERS, LAYERS[oledLayer].name);
    oled.setTextSize(2);
    oled.setCursor(0, OLED_HEIGHT == 64 ? 28 : 14);
    oled.print(oledStatus);
  }
  oled.display();
}

#else
// ---------------------------------------------------------------------
//  Sem tela
// ---------------------------------------------------------------------
void displayBegin() {}
void displayLayer(uint8_t) {}
void displayKey(uint8_t, uint8_t, bool) {}
void displayStatus(const char *) {}
void displaySleep(bool) {}
void displayTask() {}
#endif
