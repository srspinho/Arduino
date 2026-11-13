// =====================================================================
//  Tela: LCD ST7789 colorido (240x240 ou 172x320) ou OLED SSD1306
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

// Layout deitado (ou quadrado): 320 x 172 ou 240 x 240
static const int16_t SCR_W = LCD_WIDTH > LCD_HEIGHT ? LCD_WIDTH : LCD_HEIGHT;
static const int16_t SCR_H = LCD_WIDTH > LCD_HEIGHT ? LCD_HEIGHT : LCD_WIDTH;
static const int16_t HEAD_H = 26;                              // cabecalho
static const int16_t FOOT_H = 22;                              // rodape (encoder)
static const int16_t CELL_W = SCR_W / 4;                       // 80 ou 60
static const int16_t CELL_H = (SCR_H - HEAD_H - FOOT_H) / 3;   // 41 ou 64
static const int16_t LINE_H = 16;                              // altura de uma linha de texto

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

// Copia "src" cortando o final ate caber em maxW pixels
static void fitText(Adafruit_GFX &g, const char *src, const GFXfont *f, int16_t maxW, char *out, size_t outSize) {
  strncpy(out, src, outSize - 1);
  out[outSize - 1] = 0;
  size_t n = strlen(out);
  while (n > 0 && textWidth(g, out, f) > maxW) out[--n] = 0;
}

// Quebra um rotulo em linhas que cabem em maxW: primeiro nos espacos e depois
// das barras; uma palavra que nao cabe sozinha e hifenizada ("Desfa-" "zer").
// Retorna o numero de linhas, ou 0 se precisar de mais que maxLines.
static const uint8_t MAX_LINES = 4;
static uint8_t wrapLabel(Adafruit_GFX &g, const char *txt, const GFXfont *f, int16_t maxW,
                         uint8_t maxLines, char lines[MAX_LINES][24], bool allowHyphen) {
  uint8_t n = 0;
  char cur[24] = "";
  const char *p = txt;
  while (*p) {
    // pega a proxima palavra (ate espaco, ou incluindo a barra)
    char word[24];
    size_t k = 0;
    while (*p && *p != ' ' && k < sizeof(word) - 1) {
      word[k++] = *p;
      if (*p++ == '/') break;
    }
    word[k] = 0;
    while (*p == ' ') p++;

    const char *w = word;
    while (*w) {
      // tenta juntar na linha atual
      char trial[48];
      bool needSpace = cur[0] && cur[strlen(cur) - 1] != '/';
      snprintf(trial, sizeof(trial), "%s%s%s", cur, needSpace ? " " : "", w);
      if (strlen(trial) < 24 && textWidth(g, trial, f) <= maxW) {
        strcpy(cur, trial);
        break;
      }
      if (cur[0]) {   // fecha a linha atual e tenta a palavra numa linha nova
        if (n >= maxLines) return 0;
        strcpy(lines[n++], cur);
        cur[0] = 0;
        continue;
      }
      // palavra sozinha nao cabe: hifeniza no maior pedaco que cabe
      if (!allowHyphen) return 0;
      size_t len = strlen(w), take = len - 1;
      char part[24];
      for (; take > 1; take--) {
        snprintf(part, sizeof(part), "%.*s-", (int)take, w);
        if (textWidth(g, part, f) <= maxW) break;
      }
      if (take <= 1) return 0;
      if (n >= maxLines) return 0;
      snprintf(lines[n++], 24, "%.*s-", (int)take, w);
      w += take;
    }
  }
  if (cur[0]) {
    if (n >= maxLines) return 0;
    strcpy(lines[n++], cur);
  }
  return n;
}

// Rotulo de uma tecla. Ordem de preferencia:
//   negrito sem hifen -> normal sem hifen -> negrito com hifen -> normal com hifen -> fonte pequena
static void drawLabel(Adafruit_GFX &g, const char *txt, int16_t cx, int16_t cy, int16_t maxW, int16_t maxH) {
  uint8_t maxLines = maxH / LINE_H;
  if (maxLines > MAX_LINES) maxLines = MAX_LINES;
  char lines[MAX_LINES][24];
  const GFXfont *fonts[4] = {F_BOLD, F_REG, F_BOLD, F_REG};
  for (uint8_t t = 0; t < 4; t++) {
    const GFXfont *f = fonts[t];
    uint8_t n = wrapLabel(g, txt, f, maxW, maxLines, lines, t >= 2);
    if (n == 0) continue;
    int16_t base = cy - (n * LINE_H) / 2 + 12;
    for (uint8_t i = 0; i < n; i++) printLine(g, lines[i], f, cx, base + i * LINE_H);
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
  drawLabel(c, L.keys[i].type == A_NONE ? "-" : L.keys[i].label, CELL_W / 2, CELL_H / 2, CELL_W - 7, CELL_H - 6);
  tft.drawRGBBitmap((i % 4) * CELL_W, HEAD_H + (i / 4) * CELL_H, c.getBuffer(), CELL_W, CELL_H);
}

void displayStatus(const char *status) {
  const Layer &L = LAYERS[lcdLayer];
  GFXcanvas16 &c = headCanvas;
  c.fillScreen(layerColor(L));
  c.setTextColor(layerTextColor(L));
  printLine(c, L.name, F_BOLD, 6, 19, -1);
  // a direita: "ultima acao  1/3", encurtando se nao couber
  int16_t avail = SCR_W - 12 - textWidth(c, L.name, F_BOLD) - 12;
  char buf[32];
  snprintf(buf, sizeof(buf), "%s   %u/%u", status, lcdLayer + 1, NUM_LAYERS);
  if (textWidth(c, buf, F_REG) > avail) fitText(c, status, F_REG, avail, buf, sizeof(buf));
  printLine(c, buf, F_REG, SCR_W - 6, 19, 1);
  tft.drawRGBBitmap(0, 0, c.getBuffer(), SCR_W, HEAD_H);
}

static void drawFooter(const Layer &L) {
  GFXcanvas16 &c = footCanvas;
  char buf[32], fit[28];
  c.fillScreen(C_BG);
  // no centro, o clique; nos lados, o giro (encurtados para nao encostar)
  fitText(c, L.encClick.label, F_BOLD, SCR_W / 3 - textWidth(c, "[]", F_BOLD), fit, sizeof(fit));
  snprintf(buf, sizeof(buf), "[%s]", fit);
  strcpy(fit, buf);
  int16_t side = (SCR_W - textWidth(c, fit, F_BOLD)) / 2 - 10;
  c.setTextColor(layerColor(L));
  printLine(c, fit, F_BOLD, SCR_W / 2, 16);
  c.setTextColor(C_DIM);
  fitText(c, L.encLeft.label, F_REG, side - textWidth(c, "<", F_REG), fit, sizeof(fit));
  snprintf(buf, sizeof(buf), "<%s", fit);
  printLine(c, buf, F_REG, 4, 16, -1);
  snprintf(buf, sizeof(buf), "%s>", L.encRight.label);
  if (textWidth(c, buf, F_REG) > side) {   // corta o texto mas mantem a seta
    fitText(c, L.encRight.label, F_REG, side - textWidth(c, ">", F_REG), fit, sizeof(fit));
    snprintf(buf, sizeof(buf), "%s>", fit);
  }
  printLine(c, buf, F_REG, SCR_W - 4, 16, 1);
  tft.drawRGBBitmap(0, SCR_H - FOOT_H, c.getBuffer(), SCR_W, FOOT_H);
}

// Na RP2350 o SPI liga no modo 0 (clock parado em LOW) e so passa para o
// modo 3 (clock parado em HIGH) no primeiro comando, DEPOIS do reset que a
// biblioteca faz. Essa subida do clock conta como 1 bit a mais e, num LCD
// sem pino CS (GMT130), todos os bytes seguintes chegam deslocados.
// Solucao: com o SPI ja na frequencia e no modo finais (clock parado e
// estavel), da um novo reset no LCD e repete a inicializacao.
static void lcdRealign() {
  if (LCD_RST < 0) return;
  tft.startWrite();             // aplica frequencia/modo finais no SPI
  tft.endWrite();
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  uint8_t colmod = 0x55;        // 16 bits por pixel
  tft.sendCommand(ST77XX_SLPOUT);
  delay(150);
  tft.sendCommand(ST77XX_COLMOD, &colmod, 1);
  delay(10);
  tft.sendCommand(ST77XX_NORON);
  delay(10);
  tft.sendCommand(ST77XX_DISPON);
  delay(10);
  // inversao e rotacao sao reenviadas logo depois, em displayBegin()
}

void displayBegin() {
  pinMode(LCD_BL, OUTPUT);
  setBacklight(0);
  SPI1.setSCK(LCD_SCK);
  SPI1.setTX(LCD_MOSI);
  tft.init(LCD_WIDTH, LCD_HEIGHT, LCD_SPI_MODE);
  tft.setSPISpeed(LCD_SPI_HZ);
  lcdRealign();
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

// Na RP2350 o SPI liga no modo 0 (clock parado em LOW) e so passa para o
// modo 3 (clock parado em HIGH) no primeiro comando, DEPOIS do reset que a
// biblioteca faz. Essa subida do clock conta como 1 bit a mais e, num LCD
// sem pino CS (GMT130), todos os bytes seguintes chegam deslocados.
// Solucao: com o SPI ja na frequencia e no modo finais (clock parado e
// estavel), da um novo reset no LCD e repete a inicializacao.
static void lcdRealign() {
  if (LCD_RST < 0) return;
  tft.startWrite();             // aplica frequencia/modo finais no SPI
  tft.endWrite();
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  uint8_t colmod = 0x55;        // 16 bits por pixel
  tft.sendCommand(ST77XX_SLPOUT);
  delay(150);
  tft.sendCommand(ST77XX_COLMOD, &colmod, 1);
  delay(10);
  tft.sendCommand(ST77XX_NORON);
  delay(10);
  tft.sendCommand(ST77XX_DISPON);
  delay(10);
  // inversao e rotacao sao reenviadas logo depois, em displayBegin()
}

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
// Na RP2350 o SPI liga no modo 0 (clock parado em LOW) e so passa para o
// modo 3 (clock parado em HIGH) no primeiro comando, DEPOIS do reset que a
// biblioteca faz. Essa subida do clock conta como 1 bit a mais e, num LCD
// sem pino CS (GMT130), todos os bytes seguintes chegam deslocados.
// Solucao: com o SPI ja na frequencia e no modo finais (clock parado e
// estavel), da um novo reset no LCD e repete a inicializacao.
static void lcdRealign() {
  if (LCD_RST < 0) return;
  tft.startWrite();             // aplica frequencia/modo finais no SPI
  tft.endWrite();
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  uint8_t colmod = 0x55;        // 16 bits por pixel
  tft.sendCommand(ST77XX_SLPOUT);
  delay(150);
  tft.sendCommand(ST77XX_COLMOD, &colmod, 1);
  delay(10);
  tft.sendCommand(ST77XX_NORON);
  delay(10);
  tft.sendCommand(ST77XX_DISPON);
  delay(10);
  // inversao e rotacao sao reenviadas logo depois, em displayBegin()
}

void displayBegin() {}
void displayLayer(uint8_t) {}
void displayKey(uint8_t, uint8_t, bool) {}
void displayStatus(const char *) {}
void displaySleep(bool) {}
void displayTask() {}
#endif
