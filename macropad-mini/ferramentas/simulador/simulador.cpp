// Simulador da tela do MiniPad no PC.
// Usa os MESMOS arquivos do firmware (config.h, gfx.h, ui.h) e grava
// os quadros como imagens .ppm, que o gerar_previa.py transforma em PNG/GIF.
//
//   g++ -O2 -std=c++17 -Istub -I../../firmware/MiniPad simulador.cpp -o simulador
//   ./simulador pasta_de_saida
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "config.h"
#include "events.h"
#include "ui.h"

static uint16_t buffer[LCD_W * LCD_H];

static void savePPM(const std::string &path) {
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) { perror(path.c_str()); exit(1); }
  fprintf(f, "P6\n%d %d\n255\n", LCD_W, LCD_H);
  for (int i = 0; i < LCD_W * LCD_H; i++) {
    uint16_t c = buffer[i];
    uint8_t rgb[3] = {gfx::red(c), gfx::green(c), gfx::blue(c)};
#if LCD_BGR
    std::swap(rgb[0], rgb[2]);
#endif
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}

struct Step { uint32_t t; Event e; };

int main(int argc, char **argv) {
  std::string out = argc > 1 ? argv[1] : "quadros";
  gfx::fb = buffer;

  // Roteiro: o que "acontece" em cada instante (ms)
  const uint32_t T0 = 2400;
  std::vector<Step> script = {
    {T0 + 400,  {EV_KEY_DOWN, 1, 0}}, {T0 + 560, {EV_KEY_UP, 1, 0}},
    {T0 + 1900, {EV_ENC_TURN, 0, 1}}, {T0 + 2050, {EV_ENC_TURN, 0, 1}}, {T0 + 2200, {EV_ENC_TURN, 0, 1}},
    {T0 + 3600, {EV_ENC_CLICK, 0, 0}},
    {T0 + 4900, {EV_ENC_HOLD, 0, 1}},
    {T0 + 5500, {EV_LAYER, 1, 1}},
    {T0 + 6000, {EV_ENC_HOLD, 0, 0}},
    {T0 + 6900, {EV_KEY_DOWN, 3, 0}}, {T0 + 7100, {EV_KEY_UP, 3, 0}},
    {T0 + 8600, {EV_LAYER, 2, 1}},
    {T0 + 9500, {EV_KEY_DOWN, 4, 0}}, {T0 + 9650, {EV_KEY_UP, 4, 0}},
    {T0 + 11000, {EV_LAYER, 3, 1}},
    {T0 + 12000, {EV_KEY_DOWN, 0, 0}}, {T0 + 12150, {EV_KEY_UP, 0, 0}},
  };
  const uint32_t END = T0 + 13600;

  // Quadros "fotografados" para as imagens fixas
  struct Shot { uint32_t t; const char *name; };
  std::vector<Shot> shots = {
    {700, "abertura"},
    {T0 + 200, "midia"},
    {T0 + 520, "tecla"},
    {T0 + 2320, "encoder"},
    {T0 + 5300, "segurando"},
    {T0 + 5620, "trocando"},
    {T0 + 7000, "reuniao"},
    {T0 + 9580, "edicao"},
    {T0 + 12080, "f13"},
  };

  ui::begin(0);
  size_t next = 0;
  int frameNo = 0;
  for (uint32_t now = 0; now <= END; now += 20) {   // 50 quadros por segundo
    while (next < script.size() && script[next].t <= now) ui::handle(script[next++].e, now);
    ui::frame(now);
    if (frameNo % 2 == 0) {   // GIF a 25 quadros por segundo
      char name[64];
      snprintf(name, sizeof(name), "/anim_%05d.ppm", frameNo / 2);
      savePPM(out + name);
    }
    for (auto &s : shots)
      if (s.t >= now && s.t < now + 20) savePPM(out + "/" + s.name + ".ppm");
    frameNo++;
  }

  // Descanso de tela (pula o tempo)
  uint32_t t = END + SAVER_AFTER_MS + 100;
  for (int i = 0; i < 60; i++, t += 20) ui::frame(t);
  savePPM(out + "/descanso.ppm");
  printf("ok: %d quadros\n", frameNo);
  return 0;
}
