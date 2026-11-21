// =====================================================================
//  MiniPad: 5 teclas, encoder e LCD ST7735S. CONFIGURACAO
//  Edite este arquivo para mudar pinos, tela, cores e camadas.
// =====================================================================
#pragma once
#include <Keyboard.h>
#include "actions.h"

// ---------------------------------------------------------------------
//  PINOS (numeracao GPxx do RP2040, placa Waveshare RP2040-Zero)
//
//  Lado das teclas da placa:   GP0 ... GP8
//  Ponta de baixo da placa:    GP9 GP10 GP11 GP12 GP13
//  Sobram livres: GP14, GP15, GP26 a GP29
// ---------------------------------------------------------------------

// 5 teclas ligadas direto: um lado do switch no GPIO, o outro no GND.
// Ordem: esquerda -> direita.
#define NUM_KEYS 5
static const uint8_t KEY_PINS[NUM_KEYS] = {0, 1, 2, 3, 4};

// Encoder EC11 (pino do meio "C" no GND)
#define ENC_PIN_A   5
#define ENC_PIN_B   6
#define ENC_PIN_SW  7    // botao do encoder (o outro lado no GND)

// Quantas transicoes o encoder gera por clique (detent).
// EC11 comum = 4. Se cada clique contar 2 passos, use 8;
// se precisar de 2 cliques para 1 passo, use 2.
#define ENC_STEPS_PER_DETENT 4
#define ENC_REVERSE 0    // 1 inverte o sentido do giro

// ---------------------------------------------------------------------
//  TELA ST7735S (SPI de 4 fios, porta SPI1)
//
//  Ligacao (nomes impressos no modulo):
//    GND -> GND           VCC -> 3V3  (NAO use 5V se o modulo nao tiver regulador)
//    SCL/SCK -> GP10      SDA/MOSI -> GP11   (sao SPI, nao I2C!)
//    RES/RST -> GP12      DC/A0 -> GP8       CS -> GP9
//    BLK/BL/LED -> GP13   (brilho por PWM)
// ---------------------------------------------------------------------
#define LCD_SCK   10
#define LCD_MOSI  11
#define LCD_CS    9
#define LCD_DC    8
#define LCD_RST   12
#define LCD_BL    13
#define LCD_BL_ACTIVE_LOW 0   // 1 se a luz de fundo so acender com BL em LOW

// Modelo do modulo:
//   LCD_180 -> 1.8"  128x160 (o mais comum, placa vermelha de 8 pinos)
//   LCD_096 -> 0.96" 80x160 IPS
//   LCD_144 -> 1.44" 128x128
#define LCD_180 180
#define LCD_096 96
#define LCD_144 144
#ifndef LCD_MODEL
#define LCD_MODEL LCD_180
#endif

// Se a imagem sair deslocada, com borda de "lixo" ou cores estranhas,
// troque a versao de inicializacao (sao variacoes do mesmo chip):
//   1.8":  INITR_BLACKTAB (padrao), INITR_GREENTAB, INITR_REDTAB
//   0.96": INITR_MINI160x80 (padrao), INITR_MINI160x80_PLUGIN
//   1.44": INITR_144GREENTAB
#if LCD_MODEL == LCD_180
#define LCD_INIT     INITR_BLACKTAB
#define LCD_W        160      // largura DEITADO (depois da rotacao)
#define LCD_H        128
#elif LCD_MODEL == LCD_096
#define LCD_INIT     INITR_MINI160x80
#define LCD_W        160
#define LCD_H        80
#else
#define LCD_INIT     INITR_144GREENTAB
#define LCD_W        128
#define LCD_H        128
#endif
#define LCD_ROTATION 1        // 1 ou 3 (deitado). Se sair de cabeca para baixo, use 3
#define LCD_BGR      0        // 1 se vermelho e azul aparecerem trocados
#define LCD_INVERT   0        // 1 se as cores aparecerem em "negativo"
#define LCD_SPI_HZ   32000000 // baixe para 16000000 se a imagem vier com falhas (fios longos)
#define LCD_USE_DMA  1        // 0 = envia sem DMA (mais lento, para teste)

#define LCD_BRIGHTNESS 220    // brilho normal (0-255)
#define LCD_DIM        40     // brilho no descanso de tela

// ---------------------------------------------------------------------
//  LED RGB da propria RP2040-Zero (GP16)
//  Respira na cor da camada e pisca em branco a cada tecla.
// ---------------------------------------------------------------------
#define USE_BOARD_LED  1
#define BOARD_LED_PIN  16
// O LED da RP2040-Zero costuma ser RGB. Se o vermelho aparecer verde, use NEO_GRB.
#define BOARD_LED_ORDER NEO_RGB
#define BOARD_LED_MAX   60    // brilho maximo 0-255 (o LED e bem forte)

// ---------------------------------------------------------------------
//  ANIMACOES E TEMPOS
// ---------------------------------------------------------------------
#define FPS              50        // quadros por segundo (o envio leva ~10 ms por quadro)
#define BOOT_ANIMATION   1         // animacao de abertura
#define SAVER_AFTER_MS   (60UL * 1000UL)         // descanso de tela (estrelas) depois de 1 min
#define SLEEP_AFTER_MS   (10UL * 60UL * 1000UL)  // apaga a tela depois de 10 min (0 = nunca)
#define WAKE_EATS_KEY    1         // 1 = com a tela apagada, a 1a tecla so acorda (nao digita)

#define DEBOUNCE_MS      5
#define DEBUG_SERIAL     1         // mensagens no Monitor Serial (115200). 0 = desliga

// ---------------------------------------------------------------------
//  CAMADAS
//  Segure o botao do encoder e gire para trocar de camada.
//
//  Tipos de acao (veja actions.h):
//    KEYS("Nome", tecla1, tecla2, ...)  -> atalho (ate 4 teclas juntas)
//    MEDIA("Nome", codigo)              -> tecla de midia (volume, play...)
//    TEXT("Nome", "texto")              -> digita um texto
//    LAYER_NEXT                         -> vai para a proxima camada
//    NONE                               -> nada
//
//  Os NOMES podem ter acento (aparecem certinho na tela).
//  Se o nome comecar com um emoji da lista de ferramentas/gerar_icones.py
//  (ex.: "⏯ Play/Pause"), a tecla mostra o icone colorido na tela;
//  sem emoji, mostra o proprio nome (use nomes curtos, como "F13").
//  O TEXT digita como teclado americano (US): use so letras sem acento
//  e numeros, ou configure o Windows como "US Internacional".
// ---------------------------------------------------------------------

struct Layer {
  const char *name;
  uint8_t r, g, b;          // cor da camada (tela e LED)
  Action keys[NUM_KEYS];
  Action encLeft;           // girar para a esquerda
  Action encRight;          // girar para a direita
  Action encClick;          // clicar o encoder
};

static const Layer LAYERS[] = {
  // ---------------- Midia ----------------
  {
    "Mídia", 0, 170, 255,
    {
      MEDIA("⏮ Anterior",   KEY_SCAN_PREVIOUS),
      MEDIA("⏯ Play/Pause", KEY_PLAY_PAUSE),
      MEDIA("⏭ Próxima",    KEY_SCAN_NEXT),
      KEYS("🖥 Desktop",     KEY_LEFT_GUI, 'd'),
      MEDIA("🧮 Calculadora", KEY_AL_CALCULATOR),
    },
    MEDIA("🔉 Volume −", KEY_VOLUME_DECREMENT),
    MEDIA("🔊 Volume +", KEY_VOLUME_INCREMENT),
    MEDIA("🔇 Mudo",     KEY_MUTE),
  },

  // ---------------- Reuniao (Microsoft Teams) ----------------
  // No Google Meet: microfone = Ctrl+D, camera = Ctrl+E
  {
    "Reunião", 140, 90, 255,
    {
      KEYS("🎤 Microfone", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'm'),
      KEYS("📷 Câmera",    KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'o'),
      KEYS("✋ Levantar mão", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'k'),
      KEYS("💻 Compartilhar", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'e'),
      KEYS("🚪 Sair",      KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'h'),
    },
    MEDIA("🔉 Volume −", KEY_VOLUME_DECREMENT),
    MEDIA("🔊 Volume +", KEY_VOLUME_INCREMENT),
    KEYS("🎤 Microfone", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'm'),
  },

  // ---------------- Edicao ----------------
  {
    "Edição", 255, 140, 0,
    {
      KEYS("📄 Copiar",   KEY_LEFT_CTRL, 'c'),
      KEYS("📋 Colar",    KEY_LEFT_CTRL, 'v'),
      KEYS("✂ Recortar", KEY_LEFT_CTRL, 'x'),
      KEYS("💾 Salvar",   KEY_LEFT_CTRL, 's'),
      KEYS("📸 Print",    KEY_LEFT_GUI, KEY_LEFT_SHIFT, 's'),
    },
    KEYS("↩ Desfazer", KEY_LEFT_CTRL, 'z'),
    KEYS("↪ Refazer",  KEY_LEFT_CTRL, 'y'),
    KEYS("🔒 Bloquear", KEY_LEFT_GUI, 'l'),
  },

  // ---------------- F13-F20 ----------------
  // Teclas que nao existem no teclado normal: otimas para atalhos
  // globais no OBS, Discord, Photoshop, AutoHotkey etc.
  {
    "F13–F20", 0, 220, 130,
    {
      KEYS("F13", KEY_F13), KEYS("F14", KEY_F14), KEYS("F15", KEY_F15),
      KEYS("F16", KEY_F16), KEYS("F17", KEY_F17),
    },
    KEYS("F18", KEY_F18),
    KEYS("F19", KEY_F19),
    KEYS("F20", KEY_F20),
  },
};

#define NUM_LAYERS (sizeof(LAYERS) / sizeof(LAYERS[0]))
