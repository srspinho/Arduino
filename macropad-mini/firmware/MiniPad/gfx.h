// =====================================================================
//  Graficos em memoria (framebuffer RGB565)
//
//  Cada quadro inteiro e desenhado na RAM (160x128x2 = 40 KB) e depois
//  enviado de uma vez para o LCD por DMA. Assim da para fazer animacoes
//  suaves, transparencia e texto com anti-aliasing, sem piscar.
//  Este arquivo nao depende do Arduino: o simulador do PC usa o mesmo codigo.
// =====================================================================
#pragma once
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "fonts.h"

namespace gfx {

static const int16_t W = LCD_W;
static const int16_t H = LCD_H;

static uint16_t *fb = nullptr;               // quadro sendo desenhado

// Deslocamento e recorte (usados na transicao entre camadas)
static int16_t ox = 0, oy = 0;
static int16_t clipX0 = 0, clipY0 = 0, clipX1 = W, clipY1 = H;

static inline void setOrigin(int16_t x, int16_t y) { ox = x; oy = y; }
static inline void setClip(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  clipX0 = x0 < 0 ? 0 : x0;
  clipY0 = y0 < 0 ? 0 : y0;
  clipX1 = x1 > W ? W : x1;
  clipY1 = y1 > H ? H : y1;
}
static inline void resetClip() { setClip(0, 0, W, H); }

// ------------------------------ cores --------------------------------
static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
#if LCD_BGR
  uint8_t t = r; r = b; b = t;
#endif
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}
static inline uint8_t red(uint16_t c)   { uint8_t v = (c >> 8) & 0xF8; return v | (v >> 5); }
static inline uint8_t green(uint16_t c) { uint8_t v = (c >> 3) & 0xFC; return v | (v >> 6); }
static inline uint8_t blue(uint16_t c)  { uint8_t v = (c << 3) & 0xF8; return v | (v >> 5); }

// Mistura a cor "b" sobre "a" com transparencia alpha (0 = so a, 255 = so b).
// Trabalha com os 3 canais de uma vez (truque classico do RGB565).
static inline uint16_t blend(uint16_t a, uint16_t b, uint8_t alpha) {
  uint32_t al = (alpha + 4) >> 3;   // 0..32
  uint32_t x = (a | ((uint32_t)a << 16)) & 0x07E0F81F;
  uint32_t y = (b | ((uint32_t)b << 16)) & 0x07E0F81F;
  uint32_t r = (x + (((y - x) * al) >> 5)) & 0x07E0F81F;
  return (uint16_t)(r | (r >> 16));
}
// Escurece / clareia (0..255)
static inline uint16_t darken(uint16_t c, uint8_t k) { return blend(0x0000, c, k); }
static inline uint16_t lighten(uint16_t c, uint8_t k) { return blend(c, 0xFFFF, k); }

// Cor do arco-iris (h: 0..1535)
static uint16_t hue(uint16_t h) {
  h %= 1536;
  uint8_t x = h & 255;
  switch (h >> 8) {
    case 0: return rgb(255, x, 0);
    case 1: return rgb(255 - x, 255, 0);
    case 2: return rgb(0, 255, x);
    case 3: return rgb(0, 255 - x, 255);
    case 4: return rgb(x, 0, 255);
    default: return rgb(255, 0, 255 - x);
  }
}

// Texto preto ou branco, o que ler melhor sobre a cor c
static inline uint16_t contrast(uint16_t c) {
  return (red(c) * 299 + green(c) * 587 + blue(c) * 114) / 1000 > 150 ? 0x0000 : 0xFFFF;
}

// ----------------------------- pixels --------------------------------
static inline void pixel(int16_t x, int16_t y, uint16_t c) {
  x += ox; y += oy;
  if (x < clipX0 || y < clipY0 || x >= clipX1 || y >= clipY1) return;
  fb[y * W + x] = c;
}
static inline void pixelA(int16_t x, int16_t y, uint16_t c, uint8_t a) {
  if (!a) return;
  x += ox; y += oy;
  if (x < clipX0 || y < clipY0 || x >= clipX1 || y >= clipY1) return;
  uint16_t &p = fb[y * W + x];
  p = a >= 250 ? c : blend(p, c, a);
}

static void clear(uint16_t c) {
  for (int32_t i = 0; i < (int32_t)W * H; i++) fb[i] = c;
}

static void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c, uint8_t a = 255) {
  x += ox; y += oy;
  int16_t x0 = x < clipX0 ? clipX0 : x, x1 = x + w > clipX1 ? clipX1 : x + w;
  int16_t y0 = y < clipY0 ? clipY0 : y, y1 = y + h > clipY1 ? clipY1 : y + h;
  if (x0 >= x1 || y0 >= y1 || !a) return;
  for (int16_t yy = y0; yy < y1; yy++) {
    uint16_t *p = fb + yy * W + x0;
    if (a >= 250) for (int16_t n = x1 - x0; n; n--) *p++ = c;
    else for (int16_t n = x1 - x0; n; n--, p++) *p = blend(*p, c, a);
  }
}

// Degrade vertical (cima -> baixo)
static void gradientV(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t top, uint16_t bottom) {
  for (int16_t i = 0; i < h; i++) fillRect(x, y + i, w, 1, blend(top, bottom, h > 1 ? i * 255 / (h - 1) : 0));
}
// Degrade horizontal (esquerda -> direita)
static void gradientH(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t left, uint16_t right) {
  for (int16_t i = 0; i < w; i++) fillRect(x + i, y, 1, h, blend(left, right, w > 1 ? i * 255 / (w - 1) : 0));
}

// Retangulo com cantos arredondados e suavizados
static void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c, uint8_t a = 255) {
  if (r * 2 > w) r = w / 2;
  if (r * 2 > h) r = h / 2;
  fillRect(x, y + r, w, h - 2 * r, c, a);
  for (int16_t i = 0; i < r; i++) {
    // linha i dos cantos: largura suavizada
    float dy = r - i - 0.5f;
    float span = sqrtf((float)r * r - dy * dy);   // meia largura do arco nesta linha
    int16_t full = (int16_t)span;
    uint8_t edge = (uint8_t)((span - full) * a);
    int16_t inner = w - 2 * r;
    for (int side = 0; side < 2; side++) {
      int16_t yy = side ? y + h - 1 - i : y + i;
      fillRect(x + r - full, yy, inner + 2 * full, 1, c, a);
      pixelA(x + r - full - 1, yy, c, edge);
      pixelA(x + w - r + full, yy, c, edge);
    }
  }
}

// Contorno arredondado (1 px)
static void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c, uint8_t a = 255) {
  fillRect(x + r, y, w - 2 * r, 1, c, a);
  fillRect(x + r, y + h - 1, w - 2 * r, 1, c, a);
  fillRect(x, y + r, 1, h - 2 * r, c, a);
  fillRect(x + w - 1, y + r, 1, h - 2 * r, c, a);
  // cantos: pixels com cobertura pela distancia ao centro do arco
  for (int16_t i = 0; i < r; i++)
    for (int16_t j = 0; j < r; j++) {
      float dx = r - j - 0.5f, dy = r - i - 0.5f;
      float d = fabsf(sqrtf(dx * dx + dy * dy) - (r - 0.5f));
      if (d >= 1) continue;
      uint8_t al = (uint8_t)((1 - d) * a);
      pixelA(x + j, y + i, c, al);
      pixelA(x + w - 1 - j, y + i, c, al);
      pixelA(x + j, y + h - 1 - i, c, al);
      pixelA(x + w - 1 - j, y + h - 1 - i, c, al);
    }
}

// Circulo cheio suavizado (centro e raio em float para mover suave)
static void fillCircle(float cx, float cy, float r, uint16_t c, uint8_t a = 255) {
  if (r <= 0) return;
  int16_t x0 = (int16_t)floorf(cx - r - 1), x1 = (int16_t)ceilf(cx + r + 1);
  int16_t y0 = (int16_t)floorf(cy - r - 1), y1 = (int16_t)ceilf(cy + r + 1);
  float rin = r - 0.5f > 0 ? r - 0.5f : 0, rout = r + 0.5f;
  float rin2 = rin * rin, rout2 = rout * rout;
  for (int16_t y = y0; y <= y1; y++) {
    float dy = y + 0.5f - cy;
    for (int16_t x = x0; x <= x1; x++) {
      float dx = x + 0.5f - cx, d2 = dx * dx + dy * dy;
      if (d2 >= rout2) continue;
      if (d2 <= rin2) pixelA(x, y, c, a);
      else pixelA(x, y, c, (uint8_t)((rout - sqrtf(d2)) * a));
    }
  }
}

// Anel suavizado (circulo vazado), r = raio do meio da linha
static void ring(float cx, float cy, float r, float thick, uint16_t c, uint8_t a = 255) {
  float half = thick / 2;
  int16_t x0 = (int16_t)floorf(cx - r - half - 1), x1 = (int16_t)ceilf(cx + r + half + 1);
  int16_t y0 = (int16_t)floorf(cy - r - half - 1), y1 = (int16_t)ceilf(cy + r + half + 1);
  float lo = r - half - 0.5f, hi = r + half + 0.5f;
  float lo2 = lo > 0 ? lo * lo : 0, hi2 = hi * hi;
  for (int16_t y = y0; y <= y1; y++) {
    float dy = y + 0.5f - cy;
    for (int16_t x = x0; x <= x1; x++) {
      float dx = x + 0.5f - cx, d2 = dx * dx + dy * dy;
      if (d2 >= hi2 || d2 <= lo2) continue;
      float cov = half + 0.5f - fabsf(sqrtf(d2) - r);
      if (cov > 1) cov = 1;
      pixelA(x, y, c, (uint8_t)(cov * a));
    }
  }
}

// Arco: "carimba" pontinhos ao longo do angulo (graus, 0 = em cima, sentido horario)
static void arc(float cx, float cy, float r, float thick, float deg0, float deg1, uint16_t c, uint8_t a = 255) {
  if (deg1 < deg0) { float t = deg0; deg0 = deg1; deg1 = t; }
  float step = 40.0f / (r > 1 ? r : 1);   // ~0.7 px entre pontos
  for (float d = deg0; d <= deg1; d += step) {
    float rad = d * 0.0174533f;
    fillCircle(cx + sinf(rad) * r, cy - cosf(rad) * r, thick / 2, c, a);
  }
}

// Linha grossa suavizada (pontinhos ao longo do caminho)
static void line(float x0, float y0, float x1, float y1, float thick, uint16_t c, uint8_t a = 255) {
  float dx = x1 - x0, dy = y1 - y0;
  float len = sqrtf(dx * dx + dy * dy);
  int n = (int)(len / 0.7f) + 1;
  for (int i = 0; i <= n; i++) {
    float t = (float)i / n;
    fillCircle(x0 + dx * t, y0 + dy * t, thick / 2, c, a);
  }
}

// ------------------------------ texto --------------------------------
// Le o proximo caractere de um texto UTF-8 (para aceitar acentos e emojis)
static uint32_t nextChar(const char *&s) {
  uint8_t c = (uint8_t)*s++;
  if (c < 0x80) return c;
  int extra = (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : 0;
  uint32_t u = c & (0x3F >> extra);
  for (int i = 0; i < extra; i++) {
    if ((*s & 0xC0) != 0x80) return '?';
    u = (u << 6) | ((uint8_t)*s++ & 0x3F);
  }
  return extra ? u : '?';
}

// Seletor de variacao de emoji (U+FE0F): invisivel, e ignorado
static inline bool invisible(uint32_t code) { return code == 0xFE0F || code == 0x200D; }

static const Glyph *findGlyph(const Font &f, uint32_t code) {
  int lo = 0, hi = f.count - 1;
  while (lo <= hi) {
    int mid = (lo + hi) / 2;
    uint16_t m = f.glyphs[mid].code;
    if (m == code) return &f.glyphs[mid];
    if (m < code) lo = mid + 1; else hi = mid - 1;
  }
  return code == '?' ? nullptr : findGlyph(f, '?');
}

static int16_t textWidth(const Font &f, const char *s) {
  int16_t w = 0;
  while (*s) {
    uint32_t code = nextChar(s);
    if (invisible(code)) continue;
    const Glyph *g = findGlyph(f, code);
    if (g) w += g->adv;
  }
  return w;
}

// Funcao de cor por pixel (para texto em degrade). Recebe x e y na tela.
typedef uint16_t (*ColorFn)(int16_t x, int16_t y, void *ctx);

// Escreve texto com a linha de base em y. Retorna o x final.
static int16_t text(const Font &f, int16_t x, int16_t y, const char *s, uint16_t c,
                    uint8_t alpha = 255, ColorFn fn = nullptr, void *ctx = nullptr) {
  while (*s) {
    uint32_t code = nextChar(s);
    if (invisible(code)) continue;
    const Glyph *g = findGlyph(f, code);
    if (!g) continue;
    const uint8_t *bits = f.bits + g->offset;
    int16_t stride = (g->w + 1) / 2;
    int16_t gx = x + g->xoff, gy = y + g->yoff;
    for (int16_t j = 0; j < g->h; j++) {
      for (int16_t i = 0; i < g->w; i++) {
        uint8_t v = bits[j * stride + i / 2];
        v = (i & 1) ? (v & 0x0F) : (v >> 4);
        if (!v) continue;
        uint16_t col = fn ? fn(gx + i, gy + j, ctx) : c;
        pixelA(gx + i, gy + j, col, (uint8_t)((v * 17 * alpha) >> 8));
      }
    }
    x += g->adv;
  }
  return x;
}

// align: 0 = centralizado em x, -1 = comeca em x, 1 = termina em x
static int16_t textAligned(const Font &f, int16_t x, int16_t y, const char *s, int align, uint16_t c,
                           uint8_t alpha = 255, ColorFn fn = nullptr, void *ctx = nullptr) {
  int16_t w = textWidth(f, s);
  int16_t left = align == 0 ? x - w / 2 : (align < 0 ? x : x - w);
  return text(f, left, y, s, c, alpha, fn, ctx);
}

}  // namespace gfx
