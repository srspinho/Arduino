// Tipos de acao usados no config.h
#pragma once
#include <stdint.h>

enum ActionType : uint8_t {
  A_NONE,
  A_KEYS,        // atalho: mantem as teclas pressionadas enquanto a tecla fisica estiver pressionada
  A_MEDIA,       // tecla de midia (Consumer Control)
  A_TEXT,        // digita um texto
  A_LAYER_NEXT,  // proxima camada
};

struct Action {
  ActionType type;
  uint8_t keys[4];
  uint16_t media;
  const char *text;
  const char *label;
};

#define KEYS(lbl, ...)  { A_KEYS,  { __VA_ARGS__ }, 0, nullptr, lbl }
#define MEDIA(lbl, c)   { A_MEDIA, { 0 }, (uint16_t)(c), nullptr, lbl }
#define TEXT(lbl, s)    { A_TEXT,  { 0 }, 0, s, lbl }
#define LAYER_NEXT      { A_LAYER_NEXT, { 0 }, 0, nullptr, "Camada" }
#define NONE            { A_NONE,  { 0 }, 0, nullptr, "" }
