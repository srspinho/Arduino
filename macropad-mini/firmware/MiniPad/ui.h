// =====================================================================
//  Interface e animacoes (roda no nucleo 1)
//
//   +--------------------------------------+
//   | Midia                        o o o o |  cabecalho na cor da camada
//   |  .---.   🔉 Volume -                  |
//   | ( \ o )  🔊 Volume +                  |  dial do encoder + dicas
//   |  '---'   🔇 Mudo                      |  (ou a ultima acao, grande)
//   | [⏮] [⏯] [⏭] [🖥] [🧮]               |  as 5 teclas
//   +--------------------------------------+
//
//  ui::begin(now), ui::handle(evento, now), ui::frame(now)
//  Nada aqui depende do hardware: o simulador do PC usa o mesmo arquivo.
// =====================================================================
#pragma once
#include "gfx.h"
#include "icons.h"
#include "events.h"

namespace ui {
using namespace gfx;

// ------------------------------ layout --------------------------------
static const int16_t HEAD_H = H >= 120 ? 18 : 14;
static const int16_t KEYS_H = H >= 120 ? 34 : 28;
static const int16_t KEYS_Y = H - KEYS_H;
static const int16_t HERO_Y = HEAD_H;
static const int16_t HERO_H = KEYS_Y - HERO_Y;
static const int16_t CELL_W = W / NUM_KEYS;
static const int16_t DIAL_MAX = W >= 150 ? 22 : 15;     // tela estreita: dial menor
static const int16_t DIAL_R = HERO_H / 2 - 6 > DIAL_MAX ? DIAL_MAX : HERO_H / 2 - 6;
static const int16_t DIAL_CX = (W >= 150 ? 7 : 5) + DIAL_R;
static const int16_t DIAL_CY = HERO_Y + HERO_H / 2;
static const int16_t TXT_X = DIAL_CX + DIAL_R + (W >= 150 ? 9 : 6);
static const int16_t TXT_W = W - TXT_X - 4;

// ------------------------------ tempos --------------------------------
static const uint32_t BOOT_MS = 1900;
static const uint32_t SLIDE_MS = 320;      // troca de camada
static const uint32_t MSG_MS = 1500;       // quanto tempo a ultima acao fica na tela
static const uint32_t MSG_FADE_MS = 280;

// ------------------------------ cores ---------------------------------
static const uint16_t C_BLACK = 0x0000;
static const uint16_t C_WHITE = 0xFFFF;
static inline uint16_t cTrack() { return rgb(48, 52, 64); }
static inline uint16_t cCell()  { return rgb(24, 27, 36); }
static inline uint16_t cText()  { return rgb(205, 210, 222); }
static inline uint16_t cDim()   { return rgb(120, 126, 140); }

static inline uint16_t accentOf(uint8_t layer) {
  const Layer &L = LAYERS[layer];
  return rgb(L.r, L.g, L.b);
}

// ----------------------------- utilidades -----------------------------
static inline float clamp01(float t) { return t < 0 ? 0 : (t > 1 ? 1 : t); }
static inline float easeOutCubic(float t) { t = 1 - clamp01(t); return 1 - t * t * t; }
static inline float easeInOut(float t) { t = clamp01(t); return t * t * (3 - 2 * t); }
static inline float easeOutBack(float t) {
  t = clamp01(t) - 1;
  return 1 + t * t * (2.7f * t + 1.7f);
}

static uint32_t rngState = 0x1234567;
static inline uint32_t rnd() {
  rngState ^= rngState << 13;
  rngState ^= rngState >> 17;
  rngState ^= rngState << 5;
  return rngState;
}
static inline float frand(float a, float b) { return a + (b - a) * (rnd() & 0xFFFF) / 65535.0f; }

// Icone + texto de um rotulo ("⏯ Play/Pause" -> icone ⏯ e texto "Play/Pause")
struct Label {
  const Icon *big;     // 22 px (ou nullptr)
  const Icon *small;   // 14 px (ou nullptr)
  const char *text;
};

static const Icon *findIcon(const Icon *list, uint16_t count, uint32_t code) {
  int lo = 0, hi = count - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    if (list[mid].code == code) return &list[mid];
    if (list[mid].code < code) lo = mid + 1; else hi = mid - 1;
  }
  return nullptr;
}

static Label parseLabel(const char *s) {
  Label l = {nullptr, nullptr, s ? s : ""};
  if (!s || (uint8_t)s[0] < 0x80) return l;
  const char *p = s;
  uint32_t code = nextChar(p);
  const Icon *big = findIcon(ICONS_L, ICONS_L_COUNT, code);
  if (!big) return l;
  l.big = big;
  l.small = findIcon(ICONS_S, ICONS_S_COUNT, code);
  const char *q = p;
  if (nextChar(q) == 0xFE0F) p = q;   // pula o seletor de emoji
  while (*p == ' ') p++;
  l.text = p;
  return l;
}

static void drawIcon(const Icon *ic, uint8_t size, int16_t x, int16_t y, uint8_t alpha = 255) {
  if (!ic) return;
  for (int16_t j = 0; j < size; j++)
    for (int16_t i = 0; i < size; i++) {
      uint8_t a = ic->alpha[j * size + i];
      if (!a) continue;
      uint16_t c = ic->color[j * size + i];
#if LCD_BGR
      c = (c & 0x07E0) | (c >> 11) | (c << 11);
#endif
      pixelA(x + i, y + j, c, (uint8_t)((a * alpha) >> 8));
    }
}

// Copia o texto cortando com "…" ate caber em maxW
static const char *fitText(const Font &f, const char *src, int16_t maxW, char *buf, size_t n) {
  if (textWidth(f, src) <= maxW) return src;
  strncpy(buf, src, n - 4);
  buf[n - 4] = 0;
  size_t len = strlen(buf);
  while (len > 0) {
    // volta ate o inicio de um caractere UTF-8
    do { len--; } while (len > 0 && ((uint8_t)buf[len] & 0xC0) == 0x80);
    strcpy(buf + len, "…");
    if (textWidth(f, buf) <= maxW) break;
  }
  return buf;
}

// Quebra em ate 2 linhas (no espaco ou depois da "/"). Retorna 1 ou 2, ou 0 se nao couber.
static uint8_t wrap2(const Font &f, const char *src, int16_t maxW, char *l1, char *l2, size_t n) {
  if (textWidth(f, src) <= maxW) { strncpy(l1, src, n - 1); l1[n - 1] = 0; l2[0] = 0; return 1; }
  size_t len = strlen(src);
  for (size_t cut = len; cut > 0; cut--) {
    char c = src[cut - 1];
    if (c != ' ' && c != '/') continue;
    size_t a = c == ' ' ? cut - 1 : cut;
    if (a >= n) continue;
    memcpy(l1, src, a);
    l1[a] = 0;
    strncpy(l2, src + cut, n - 1);
    l2[n - 1] = 0;
    if (textWidth(f, l1) <= maxW && textWidth(f, l2) <= maxW) return 2;
  }
  return 0;
}

// ------------------------------ estado --------------------------------
struct Particle {
  float x, y, vx, vy, size;
  uint16_t color;
  uint16_t life, age;   // ms
};
static const uint8_t MAX_PARTICLES = 64;
static Particle parts[MAX_PARTICLES];

static uint8_t layer = 0;
static uint8_t prevLayer = 0xFF;           // 0xFF = tela preta (depois da abertura)
static int8_t slideDir = 1;
static uint32_t slideAt = 0;

static float keyGlow[NUM_KEYS];
static bool keyDown[NUM_KEYS];

static float dialAngle = 0, dialTarget = 0;   // graus
static float dialTrail = 0;                   // rastro luminoso (graus)
static int8_t dialDir = 1;
static uint32_t clickAt = 0;
static bool clickSeen = false;
static bool encHeld = false;

static const char *msgLabel = nullptr;     // ultima acao mostrada em destaque
static char msgCaption[24];
static const void *msgId = nullptr;        // para contar repeticoes (x3, x4...)
static uint16_t msgCount = 0;
static uint32_t msgAt = 0;

static uint32_t bootAt = 0;
static uint32_t lastInput = 0;
static uint32_t lastFrame = 0;
static uint32_t ledFlashAt = 0;
static bool sleeping = false;

struct Star { float x, y, z; };
static const uint8_t NUM_STARS = 70;
static Star stars[NUM_STARS];

static void spawn(float x, float y, float vx, float vy, uint16_t c, uint16_t life, float size) {
  for (Particle &p : parts)
    if (p.age >= p.life) {
      p = {x, y, vx, vy, size, c, life, 0};
      return;
    }
}

static void burst(float x, float y, uint16_t c, uint8_t n, float speed, bool upward) {
  for (uint8_t i = 0; i < n; i++) {
    float ang = upward ? frand(-2.6f, -0.55f) : frand(0, 6.283f);
    float sp = frand(speed * 0.4f, speed);
    uint16_t col = (rnd() & 3) == 0 ? C_WHITE : lighten(c, (uint8_t)frand(0, 120));
    spawn(x, y, cosf(ang) * sp, sinf(ang) * sp, col, (uint16_t)frand(350, 800), frand(0.8f, 1.9f));
  }
}

static void showMessage(const char *label, const char *caption, const void *id, uint16_t add, uint32_t now) {
  bool same = id == msgId && msgLabel && now - msgAt < MSG_MS + MSG_FADE_MS;
  msgCount = same ? msgCount + add : add;
  msgId = id;
  msgLabel = label;
  strncpy(msgCaption, caption, sizeof(msgCaption) - 1);
  msgCaption[sizeof(msgCaption) - 1] = 0;
  // repetindo a mesma acao, o texto nao "pula" de novo: so renova o tempo
  msgAt = same ? now - 200 : now;
}

// ------------------------------ eventos -------------------------------
void handle(const Event &e, uint32_t now) {
  lastInput = now;
  sleeping = false;
  uiSleeping = false;
  const Layer &L = LAYERS[layer];
  uint16_t acc = accentOf(layer);
  switch (e.type) {
    case EV_KEY_DOWN: {
      if (e.a >= NUM_KEYS) break;
      keyDown[e.a] = true;
      keyGlow[e.a] = 1;
      ledFlashAt = now;
      const Action &a = L.keys[e.a];
      if (a.type != A_NONE && a.type != A_LAYER_NEXT) {
        char cap[16];
        snprintf(cap, sizeof(cap), "TECLA %u", e.a + 1);
        showMessage(a.label, cap, &a, 1, now);
      }
      burst(e.a * CELL_W + CELL_W / 2.0f, KEYS_Y + 3, acc, 14, 2.6f, true);
      break;
    }
    case EV_KEY_UP:
      if (e.a < NUM_KEYS) keyDown[e.a] = false;
      break;
    case EV_ENC_TURN: {
      if (e.b == 0) break;   // giro que so acordou a tela
      dialTarget += 30.0f * e.b;
      dialDir = e.b > 0 ? 1 : -1;
      const Action &a = e.b > 0 ? L.encRight : L.encLeft;
      showMessage(a.label, e.b > 0 ? "GIROU ↻" : "GIROU ↺", &a, (uint16_t)abs(e.b), now);
      break;
    }
    case EV_ENC_CLICK:
      clickAt = now;
      clickSeen = true;
      ledFlashAt = now;
      showMessage(L.encClick.label, "CLIQUE", &L.encClick, 1, now);
      burst(DIAL_CX, DIAL_CY, acc, 18, 2.2f, false);
      break;
    case EV_ENC_HOLD:
      encHeld = e.b != 0;
      break;
    case EV_LAYER: {
      if (e.a >= NUM_LAYERS || e.a == layer) break;
      prevLayer = layer;
      layer = e.a;
      slideDir = e.b >= 0 ? 1 : -1;
      slideAt = now;
      dialTarget += 30.0f * slideDir;
      dialDir = slideDir;
      char cap[16];
      snprintf(cap, sizeof(cap), "CAMADA %u/%u", layer + 1, (unsigned)NUM_LAYERS);
      showMessage(LAYERS[layer].name, cap, &LAYERS[layer], 1, now);
      msgCount = 1;
      burst(DIAL_CX, DIAL_CY, accentOf(layer), 24, 2.8f, false);
      break;
    }
  }
}

// ------------------------------ desenho -------------------------------
static void drawHeader(uint8_t li, uint32_t now, bool live) {
  const Layer &L = LAYERS[li];
  uint16_t acc = accentOf(li);
  gradientH(0, 0, W, HEAD_H, acc, darken(acc, 80));
  fillRect(0, HEAD_H - 1, W, 1, lighten(acc, 90));
  if (live) {   // brilho que atravessa o cabecalho de tempos em tempos
    int16_t sx = (int16_t)((now % 4200) * (W + 120) / 4200) - 40;
    for (int16_t dx = -14; dx <= 14; dx++)
      fillRect(sx + dx, 0, 1, HEAD_H - 1, C_WHITE, (uint8_t)(70 - abs(dx) * 5));
  }
  uint16_t tc = contrast(acc);
  int16_t base = HEAD_H / 2 + 4;
  text(FONT_MED, 5, base, L.name, tc);
  // bolinhas das camadas
  for (uint8_t i = 0; i < NUM_LAYERS; i++) {
    float cx = W - 6 - (NUM_LAYERS - 1 - i) * 8.0f, cy = HEAD_H / 2.0f - 0.5f;
    if (i == li) fillCircle(cx, cy, 2.6f, tc);
    else ring(cx, cy, 2.2f, 1.0f, tc, 140);
  }
}

static void drawDial(uint8_t li, uint32_t now, bool live) {
  uint16_t acc = accentOf(li);
  float ang = live ? dialAngle : 0;
  float cx = DIAL_CX, cy = DIAL_CY;
  float rk = DIAL_R - 7.5f;   // raio do botao

  // marcas em volta (a mais perto do ponteiro acende)
  for (int i = 0; i < 12; i++) {
    float a = i * 30.0f;
    float diff = fmodf(fabsf(fmodf(ang, 360.0f) + 360.0f - a), 360.0f);
    if (diff > 180) diff = 360 - diff;
    float rad = a * 0.0174533f;
    uint8_t lit = diff < 30 ? (uint8_t)((30 - diff) * 8) : 0;
    fillCircle(cx + sinf(rad) * (DIAL_R + 0.5f), cy - cosf(rad) * (DIAL_R + 0.5f), 1.0f,
               blend(cTrack(), acc, lit), 255);
  }
  ring(cx, cy, DIAL_R - 3.5f, 1.6f, cTrack());

  // rastro do giro
  if (live && dialTrail > 1) {
    float t0 = ang - dialDir * dialTrail;
    arc(cx, cy, DIAL_R - 3.5f, 2.6f, t0, ang, acc, (uint8_t)(dialTrail > 60 ? 255 : 120 + dialTrail * 2));
  }

  // botao com "volume": sombra, corpo e reflexo
  fillCircle(cx + 1, cy + 1.5f, rk, rgb(6, 7, 10));
  fillCircle(cx, cy, rk, rgb(34, 38, 50));
  fillCircle(cx - 1.2f, cy - 1.2f, rk - 2.2f, rgb(46, 51, 66));
  fillCircle(cx - 2.2f, cy - 2.4f, rk * 0.45f, rgb(60, 66, 84), 120);
  ring(cx, cy, rk, 1.0f, darken(acc, encHeld && live ? 255 : 110));

  // ponteiro
  float rad = ang * 0.0174533f;
  float px = cx + sinf(rad) * rk * 0.62f, py = cy - cosf(rad) * rk * 0.62f;
  fillCircle(px, py, 4.2f, acc, 70);
  fillCircle(px, py, 2.1f, lighten(acc, 60));

  if (!live) return;
  // segurando o botao: anel pulsando (modo de troca de camada)
  if (encHeld) {
    float p = (sinf(now * 0.008f) + 1) * 0.5f;
    ring(cx, cy, DIAL_R + 3, 1.5f, acc, (uint8_t)(90 + p * 150));
  }
  // onda do clique
  uint32_t dt = now - clickAt;
  if (clickSeen && dt < 450) {
    float p = dt / 450.0f;
    ring(cx, cy, rk + p * (DIAL_R + 6 - rk), 3.0f * (1 - p) + 0.6f, lighten(acc, 80), (uint8_t)((1 - p) * 255));
  }
}

// Dicas do encoder (girar / clicar)
static void drawHints(uint8_t li, uint8_t alpha) {
  const Layer &L = LAYERS[li];
  uint16_t acc = accentOf(li);
  const Action *acts[3] = {&L.encLeft, &L.encRight, &L.encClick};
  const char *marks[3] = {"↺", "↻", "●"};
  int16_t rh = HERO_H / 3 > 16 ? 16 : HERO_H / 3;
  int16_t top = HERO_Y + (HERO_H - rh * 3) / 2;
  char buf[40];
  for (uint8_t i = 0; i < 3; i++) {
    Label lb = parseLabel(acts[i]->type == A_NONE ? "—" : acts[i]->label);
    int16_t base = top + rh * i + rh / 2 + 4;
    int16_t x = TXT_X;
    text(FONT_SMALL, x, base, marks[i], acc, alpha);
    x += 11;
    bool roomForIcon = textWidth(FONT_SMALL, lb.text) <= W - 3 - x - ICON_S_SIZE - 3;
    if (lb.small && rh >= 14 && roomForIcon) {
      drawIcon(lb.small, ICON_S_SIZE, x, base - 11, alpha);
      x += ICON_S_SIZE + 3;
    }
    text(FONT_SMALL, x, base, fitText(FONT_SMALL, lb.text, W - 3 - x, buf, sizeof(buf)), cText(), alpha);
  }
}

// Ultima acao em destaque: legenda pequena + icone + nome grande
static void drawMessage(uint8_t li, uint32_t now, uint8_t alpha) {
  uint16_t acc = accentOf(li);
  Label lb = parseLabel(msgLabel);
  float tin = (now - msgAt) / 220.0f;
  float pop = easeOutBack(tin);
  int16_t dy = (int16_t)((1 - pop) * 7);
  uint8_t a = (uint8_t)(alpha * clamp01(tin * 1.5f));

  // Tenta, nesta ordem, ate caber: fonte grande, media, media em 2 linhas,
  // pequena; primeiro com o icone ao lado, depois sem o icone.
  char l1[40], l2[40];
  const Font *f = nullptr;
  uint8_t lines = 0;
  bool useIcon = false;
  for (int withIcon = lb.big ? 1 : 0; withIcon >= 0 && !lines; withIcon--) {
    int16_t avail = TXT_W - (withIcon ? ICON_L_SIZE + 4 : 0);
    const Font *opts[3] = {&FONT_BIG, &FONT_MED, &FONT_SMALL};
    for (const Font *o : opts) {
      lines = o == &FONT_BIG ? (textWidth(*o, lb.text) <= avail ? 1 : 0)
                             : wrap2(*o, lb.text, avail, l1, l2, sizeof(l1));
      if (lines == 1 && o == &FONT_BIG) { strncpy(l1, lb.text, sizeof(l1) - 1); l1[sizeof(l1) - 1] = 0; l2[0] = 0; }
      if (lines) { f = o; useIcon = withIcon; break; }
    }
  }
  if (!lines) {   // nada coube: corta com reticencias
    f = &FONT_MED;
    useIcon = false;
    fitText(FONT_MED, lb.text, TXT_W, l1, sizeof(l1));
    l2[0] = 0;
    lines = 1;
  }
  int16_t iconW = useIcon ? ICON_L_SIZE + 4 : 0;
  int16_t lineH = f == &FONT_BIG ? 20 : (f == &FONT_MED ? 13 : 11);
  int16_t capH = HERO_H >= 50 ? 12 : 0;      // na tela de 80 px nao cabe a legenda
  int16_t blockH = capH + lines * lineH;
  int16_t top = HERO_Y + (HERO_H - blockH) / 2 + dy;

  if (capH) {
    char cap[40];
    if (msgCount > 1) snprintf(cap, sizeof(cap), "%s  ×%u", msgCaption, msgCount);
    else snprintf(cap, sizeof(cap), "%s", msgCaption);
    text(FONT_SMALL, TXT_X, top + 9, cap, lighten(acc, 40), a);
  }
  int16_t y0 = top + capH;
  int16_t tx = TXT_X;
  if (useIcon) {
    int16_t iy = y0 + (lines * lineH - ICON_L_SIZE) / 2;
    drawIcon(lb.big, ICON_L_SIZE, TXT_X, iy, a);
    tx += iconW;
  }
  int16_t asc = f == &FONT_BIG ? 15 : (f == &FONT_MED ? 10 : 8);   // altura das maiusculas
  int16_t b1 = y0 + asc + (lineH - asc) / 2;
  text(*f, tx, b1, l1, C_WHITE, a);
  if (l2[0]) text(*f, tx, b1 + lineH, l2, C_WHITE, a);
}

static void drawHero(uint8_t li, uint32_t now, bool live) {
  uint16_t acc = accentOf(li);
  gradientV(0, HERO_Y, W, HERO_H, darken(acc, 46), rgb(4, 5, 8));
  drawDial(li, now, live);

  if (live && encHeld) {   // segurando o encoder: modo de troca de camada
    int16_t base = HERO_Y + HERO_H / 2;
    text(FONT_MED, TXT_X, base - 2, "Camadas", C_WHITE);
    text(FONT_SMALL, TXT_X, base + 11, "gire para trocar", lighten(acc, 60));
    return;
  }
  uint32_t age = now - msgAt;
  if (live && msgLabel && age < MSG_MS + MSG_FADE_MS) {
    float out = age > MSG_MS ? (age - MSG_MS) / (float)MSG_FADE_MS : 0;
    drawMessage(li, now, (uint8_t)(255 * (1 - easeInOut(out))));
    if (out > 0) drawHints(li, (uint8_t)(255 * easeInOut(out)));
  } else {
    drawHints(li, 255);
  }
}

static void drawKeys(uint8_t li, bool live) {
  const Layer &L = LAYERS[li];
  uint16_t acc = accentOf(li);
  fillRect(0, KEYS_Y, W, KEYS_H, rgb(4, 5, 8));
  char l1[24], l2[24];
  for (uint8_t i = 0; i < NUM_KEYS; i++) {
    float g = live ? keyGlow[i] : 0;
    bool down = live && keyDown[i];
    int16_t x = i * CELL_W + 1 + (W - CELL_W * NUM_KEYS) / 2, y = KEYS_Y + 2;
    int16_t w = CELL_W - 2, h = KEYS_H - 4;
    if (g > 0.02f) fillRoundRect(x - 1, y - 1, w + 2, h + 2, 6, acc, (uint8_t)(g * 110));
    fillRoundRect(x, y, w, h, 5, blend(cCell(), acc, (uint8_t)(g * 220)));
    drawRoundRect(x, y, w, h, 5, acc, (uint8_t)(60 + g * 195));
    fillRect(x + 5, y + 1, w - 10, 1, C_WHITE, (uint8_t)(18 + g * 60));

    const Action &a = L.keys[i];
    int16_t push = down ? 1 : 0;   // o conteudo "afunda" 1 px com a tecla apertada
    if (a.type == A_NONE) {
      fillCircle(x + w / 2.0f, y + h / 2.0f, 1.5f, cDim());
      continue;
    }
    Label lb = parseLabel(a.label);
    if (lb.big) {
      drawIcon(lb.big, ICON_L_SIZE, x + (w - ICON_L_SIZE) / 2, y + (h - ICON_L_SIZE) / 2 + push);
    } else {
      uint16_t tc = blend(cText(), contrast(acc), (uint8_t)(g * 255));
      uint8_t n = wrap2(FONT_SMALL, lb.text, w - 3, l1, l2, sizeof(l1));
      if (!n) { fitText(FONT_SMALL, lb.text, w - 3, l1, sizeof(l1)); n = 1; }
      int16_t cy = y + h / 2 + push;
      if (n == 1) textAligned(FONT_SMALL, x + w / 2, cy + 4, l1, 0, tc);
      else {
        textAligned(FONT_SMALL, x + w / 2, cy - 1, l1, 0, tc);
        textAligned(FONT_SMALL, x + w / 2, cy + 9, l2, 0, tc);
      }
    }
  }
}

static void drawScene(uint8_t li, uint32_t now, bool live) {
  drawHero(li, now, live);
  drawKeys(li, live);
  drawHeader(li, now, live);
}

static void drawParticles() {
  for (Particle &p : parts) {
    if (p.age >= p.life) continue;
    float k = 1 - (float)p.age / p.life;
    fillCircle(p.x, p.y, p.size * (0.4f + 0.6f * k), p.color, (uint8_t)(255 * k));
  }
}

// --------------------------- abertura ---------------------------------
static uint32_t logoTime = 0;
static uint16_t rainbowFn(int16_t x, int16_t y, void *) {
  return hue((uint16_t)(x * 9 + y * 3 + logoTime * 0.9f));
}

static void drawBoot(uint32_t t) {
  clear(rgb(3, 3, 6));
  logoTime = t;
  float cx = W / 2.0f, cy = H / 2.0f - 6;
  // aneis coloridos se abrindo
  for (int i = 0; i < 5; i++) {
    float p = (t - i * 110) / 900.0f;
    if (p <= 0 || p >= 1) continue;
    float r = 4 + easeOutCubic(p) * (W * 0.75f);
    ring(cx, cy, r, 2.5f * (1 - p) + 0.5f, hue(i * 300 + t / 2), (uint8_t)((1 - p) * 220));
  }
  // logo descendo no lugar
  float pl = easeOutBack((t - 250) / 600.0f);
  if (t > 250) {
    int16_t y = (int16_t)(cy + 9 - (1 - pl) * 18);
    uint8_t a = (uint8_t)(255 * clamp01((t - 250) / 300.0f));
    textAligned(FONT_LOGO, (int16_t)cx, y, "MiniPad", 0, C_WHITE, a, rainbowFn);
  }
  if (t > 800) {
    uint8_t a = (uint8_t)(255 * clamp01((t - 800) / 400.0f));
    textAligned(FONT_SMALL, (int16_t)cx, (int16_t)cy + 26, "RP2040 • 5 teclas", 0, cDim(), a);
  }
  // escurece no final
  if (t > BOOT_MS - 300) fillRect(0, 0, W, H, C_BLACK, (uint8_t)(255 * clamp01((t - (BOOT_MS - 300)) / 300.0f)));
}

// ------------------------ descanso de tela ----------------------------
static void resetStar(Star &s, bool far) {
  s.x = frand(-1, 1);
  s.y = frand(-1, 1);
  s.z = far ? frand(0.7f, 1.0f) : frand(0.05f, 1.0f);
}

static void drawSaver(uint32_t now, float dt) {
  uint16_t acc = accentOf(layer);
  clear(rgb(2, 2, 5));
  float cx = W / 2.0f, cy = H / 2.0f;
  float speed = 0.00025f * dt;
  for (Star &s : stars) {
    float pz = s.z;
    s.z -= speed;
    if (s.z <= 0.03f) { resetStar(s, true); continue; }
    float sx = cx + s.x / s.z * W * 0.35f, sy = cy + s.y / s.z * W * 0.35f;
    float qx = cx + s.x / pz * W * 0.35f, qy = cy + s.y / pz * W * 0.35f;
    if (sx < -2 || sx > W + 2 || sy < -2 || sy > H + 2) { resetStar(s, true); continue; }
    uint8_t b = (uint8_t)(255 * clamp01(1.15f - s.z));
    uint16_t c = blend(C_WHITE, acc, 90);
    line(qx, qy, sx, sy, 1.0f + (1 - s.z) * 1.2f, c, b);
  }
  // nome da camada respirando bem fraquinho
  uint8_t a = (uint8_t)(50 + 40 * sinf(now * 0.0015f));
  textAligned(FONT_SMALL, W / 2, H - 6, LAYERS[layer].name, 0, acc, a);
}

// ------------------------------- API ----------------------------------
void begin(uint32_t now) {
  bootAt = now;
  lastInput = now;
  lastFrame = now;
  rngState ^= now * 2654435761u;
  for (Star &s : stars) resetStar(s, false);
#if BOOT_ANIMATION
  slideAt = now + BOOT_MS;
#else
  bootAt = now - BOOT_MS;
  slideAt = now;
#endif
  prevLayer = 0xFF;
  slideDir = 1;
}

static bool booting(uint32_t now) { return now - bootAt < BOOT_MS; }

// Brilho desejado da luz de fundo (0-255)
uint8_t backlight(uint32_t now) {
  if (sleeping) return 0;
  if (booting(now)) return (uint8_t)(LCD_BRIGHTNESS * clamp01((now - bootAt) / 250.0f));
  if (SAVER_AFTER_MS > 0 && now - lastInput > SAVER_AFTER_MS) return LCD_DIM;
  return LCD_BRIGHTNESS;
}

// Cor do LED da placa: respira na cor da camada e pisca em branco nas teclas
void ledColor(uint32_t now, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = g = b = 0;
  if (sleeping) return;
  const Layer &L = LAYERS[layer];
  float k = 0.30f + 0.22f * sinf(now * 0.0025f);
  if (booting(now)) {
    uint16_t c = hue(now - bootAt);
    r = red(c); g = green(c); b = blue(c);
    return;
  }
  if (SAVER_AFTER_MS > 0 && now - lastInput > SAVER_AFTER_MS) k *= 0.25f;
  float f = clamp01(1 - (now - ledFlashAt) / 250.0f);
  if (ledFlashAt == 0) f = 0;
  r = (uint8_t)(L.r * k * (1 - f) + 255 * f);
  g = (uint8_t)(L.g * k * (1 - f) + 255 * f);
  b = (uint8_t)(L.b * k * (1 - f) + 255 * f);
}

// Desenha um quadro. Retorna false quando a tela esta apagada (nao precisa enviar).
bool frame(uint32_t now) {
  float dt = (float)(now - lastFrame);
  if (dt > 60) dt = 60;
  lastFrame = now;
  float k = dt / 20.0f;   // 1.0 a 50 quadros por segundo

  if (booting(now)) {
    drawBoot(now - bootAt);
    return true;
  }

  uint32_t idle = now - lastInput;
  if (SLEEP_AFTER_MS > 0 && idle > SLEEP_AFTER_MS) {
    bool first = !sleeping;
    sleeping = true;
    uiSleeping = true;
    if (first) clear(C_BLACK);
    return first;
  }
  uiSleeping = false;
  if (SAVER_AFTER_MS > 0 && idle > SAVER_AFTER_MS) {
    drawSaver(now, dt);
    return true;
  }

  // ---- fisica das animacoes ----
  float diff = dialTarget - dialAngle;
  dialAngle += diff * (1 - powf(0.72f, k));
  float tr = fabsf(diff) * 1.4f;
  if (tr > 150) tr = 150;
  dialTrail = tr > dialTrail ? tr : dialTrail * powf(0.86f, k);
  for (uint8_t i = 0; i < NUM_KEYS; i++)
    if (!keyDown[i]) keyGlow[i] *= powf(0.84f, k);
  for (Particle &p : parts) {
    if (p.age >= p.life) continue;
    p.age += (uint16_t)dt;
    p.x += p.vx * k;
    p.y += p.vy * k;
    p.vy += 0.10f * k;      // gravidade
    p.vx *= powf(0.97f, k);
  }

  // ---- cena (deslizando na troca de camada) ----
  float sp = (now - slideAt) / (float)SLIDE_MS;
  if ((int32_t)(now - slideAt) < 0) sp = 0;
  if (sp < 1) {
    float e = easeOutCubic(sp);
    int16_t offNew = (int16_t)(slideDir * W * (1 - e));
    int16_t offOld = offNew - slideDir * W;
    if (prevLayer == 0xFF) {
      clear(C_BLACK);
    } else {
      setOrigin(offOld, 0);
      drawScene(prevLayer, now, false);
    }
    setOrigin(offNew, 0);
    drawScene(layer, now, true);
    setOrigin(0, 0);
  } else {
    drawScene(layer, now, true);
  }
  drawParticles();
  return true;
}

}  // namespace ui
