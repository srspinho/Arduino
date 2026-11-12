// =====================================================================
//  MacroPad 4x3 + encoder - CONFIGURACAO
//  Edite este arquivo para mudar pinos, teclas, cores e camadas.
// =====================================================================
#pragma once
#include <Keyboard.h>
#include "actions.h"

// ---------------------------------------------------------------------
//  PINOS (numeracao GPxx da RP2350)
//  Os pinos escolhidos existem nas bordas da Waveshare RP2350-Zero
//  e tambem na Raspberry Pi Pico 2, entao servem para as duas.
// ---------------------------------------------------------------------

// 12 teclas, ligadas direto (sem matriz, sem diodos):
// um lado do switch no GPIO, o outro no GND.
// Ordem: esquerda->direita, cima->baixo.
//
//   K1  K2  K3  K4
//   K5  K6  K7  K8
//   K9  K10 K11 K12
static const uint8_t KEY_PINS[12] = {
  0, 1, 2, 3,
  4, 5, 6, 7,
  8, 9, 10, 11
};

// Encoder EC11 (pino do meio "C" no GND)
#define ENC_PIN_A   26
#define ENC_PIN_B   27
#define ENC_PIN_SW  28   // botao do encoder (outro lado no GND)

// Quantas transicoes o encoder gera por "clique" (detent).
// EC11 comum = 4. Se cada clique contar 2 passos, use 8;
// se precisar de 2 cliques para 1 passo, use 2.
#define ENC_STEPS_PER_DETENT 4
// Inverta se o sentido do giro estiver trocado
#define ENC_REVERSE 0

// LEDs RGB WS2812B (opcionais - coloque USE_LEDS 0 se nao usar)
//  LED_PER_KEY 1 -> um LED por tecla, encadeados na ordem K1..K12:
//                   a tecla pressionada acende em branco.
//  LED_PER_KEY 0 -> fita/underglow com qualquer quantidade de LEDs:
//                   a fita toda pisca em branco a cada tecla.
#define USE_LEDS     1
#define LED_PIN      29
#define LED_COUNT    12
#define LED_PER_KEY  0
#define LED_BRIGHTNESS 40   // 0-255. Mantenha baixo se alimentar pela USB

// ---------------------------------------------------------------------
//  TELA
//    DISPLAY_LCD   -> LCD colorido 1.47" 172x320 ST7789 (GMT147SPI)
//    DISPLAY_OLED  -> OLED SSD1306 I2C 0.91"/0.96"
//    DISPLAY_NONE  -> sem tela
// ---------------------------------------------------------------------
#define DISPLAY_NONE 0
#define DISPLAY_OLED 1
#define DISPLAY_LCD  2
#define DISPLAY_TYPE DISPLAY_LCD

// --- LCD ST7789 (SPI1) ---
//  Pinos do modulo:  GND  VCC  SCL  SDA  RES  DC   CS   BL("EL")
//  Ligar em:         GND  3V3  GP14 GP15 3V3  GP12 GND  GP13
//  (SCL/SDA aqui sao o clock e o dado do SPI, nao I2C!)
//  Na Pico 2 sobram pinos: se quiser, ligue RES e CS em GPIOs e
//  coloque os numeros abaixo (-1 = ligado fixo em 3V3/GND).
#define LCD_SCK      14
#define LCD_MOSI     15
#define LCD_DC       12
#define LCD_BL       13     // brilho por PWM
#define LCD_CS       -1
#define LCD_RST      -1
#define LCD_WIDTH    172
#define LCD_HEIGHT   320
#define LCD_ROTATION 1      // 1 ou 3 (deitado). Use 3 para girar 180 graus
#define LCD_SPI_HZ   40000000
#define LCD_BRIGHTNESS 200  // 0-255
#define LCD_INVERT   1      // se as cores aparecerem invertidas, mude para 0

// --- OLED SSD1306 (I2C0) ---
#define OLED_SDA     12
#define OLED_SCL     13
#define OLED_ADDR    0x3C
#define OLED_WIDTH   128
#define OLED_HEIGHT  32     // 32 ou 64

// Tempo de debounce das teclas (ms)
#define DEBOUNCE_MS  5

// Desliga os LEDs e a tela (apaga a luz de fundo do LCD) apos X ms sem uso (0 = nunca)
#define SLEEP_AFTER_MS (10UL * 60UL * 1000UL)

// ---------------------------------------------------------------------
//  CAMADAS
//  Gire o encoder COM O BOTAO PRESSIONADO para trocar de camada.
//
//  Tipos de acao (veja actions.h):
//    KEYS("Nome", tecla1, tecla2, ...)  -> atalho (ate 4 teclas juntas)
//    MEDIA("Nome", codigo)              -> tecla de midia (volume, play...)
//    TEXT("Nome", "texto")              -> digita um texto
//    LAYER_NEXT                         -> vai para a proxima camada
//    NONE                               -> nada
//
//  Atencao ao TEXT: o firmware digita como teclado americano (US).
//  Se o seu Windows estiver em ABNT2, use so letras e numeros, ou
//  configure o layout do Windows para "US Internacional".
// ---------------------------------------------------------------------

#define NUM_LAYERS 3

struct Layer {
  const char *name;
  uint8_t r, g, b;          // cor dos LEDs nesta camada
  Action keys[12];
  Action encLeft;           // girar para a esquerda
  Action encRight;          // girar para a direita
  Action encClick;          // clicar o encoder
};

static const Layer LAYERS[NUM_LAYERS] = {
  // ---------------- Camada 0: Geral / Midia ----------------
  {
    "Geral", 0, 120, 255,
    {
      KEYS("Copiar",   KEY_LEFT_CTRL, 'c'),
      KEYS("Colar",    KEY_LEFT_CTRL, 'v'),
      KEYS("Desfazer", KEY_LEFT_CTRL, 'z'),
      KEYS("Refazer",  KEY_LEFT_CTRL, 'y'),

      MEDIA("Anterior",   KEY_SCAN_PREVIOUS),
      MEDIA("Play/Pause", KEY_PLAY_PAUSE),
      MEDIA("Proxima",    KEY_SCAN_NEXT),
      MEDIA("Calcular",   KEY_AL_CALCULATOR),

      KEYS("Print",       KEY_LEFT_GUI, KEY_LEFT_SHIFT, 's'),
      KEYS("Desktop",     KEY_LEFT_GUI, 'd'),
      KEYS("Tarefas",     KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_ESC),
      KEYS("Bloquear",    KEY_LEFT_GUI, 'l'),
    },
    MEDIA("Volume -", KEY_VOLUME_DECREMENT),
    MEDIA("Volume +", KEY_VOLUME_INCREMENT),
    MEDIA("Mudo",     KEY_MUTE),
  },

  // ---------------- Camada 1: Navegador ----------------
  {
    "Navegador", 255, 120, 0,
    {
      KEYS("Nova aba",  KEY_LEFT_CTRL, 't'),
      KEYS("Fecha aba", KEY_LEFT_CTRL, 'w'),
      KEYS("Reabrir",   KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 't'),
      KEYS("Atualizar", KEY_F5),

      KEYS("Voltar",    KEY_LEFT_ALT, KEY_LEFT_ARROW),
      KEYS("Avancar",   KEY_LEFT_ALT, KEY_RIGHT_ARROW),
      KEYS("Zoom +",    KEY_LEFT_CTRL, KEY_KP_PLUS),
      KEYS("Zoom -",    KEY_LEFT_CTRL, KEY_KP_MINUS),

      KEYS("Anonima",   KEY_LEFT_CTRL, KEY_LEFT_SHIFT, 'n'),
      KEYS("Favoritar", KEY_LEFT_CTRL, 'd'),
      KEYS("Tela cheia", KEY_F11),
      KEYS("Zoom 100%", KEY_LEFT_CTRL, KEY_KP_0),
    },
    KEYS("Aba ant.", KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_TAB),
    KEYS("Prox. aba", KEY_LEFT_CTRL, KEY_TAB),
    KEYS("Nova aba", KEY_LEFT_CTRL, 't'),
  },

  // ---------------- Camada 2: F13-F24 ----------------
  // Teclas que nao existem no teclado normal: otimas para configurar
  // atalhos globais no OBS, Discord, Photoshop, AutoHotkey etc.
  {
    "F13-F24", 180, 0, 255,
    {
      KEYS("F13", KEY_F13), KEYS("F14", KEY_F14), KEYS("F15", KEY_F15), KEYS("F16", KEY_F16),
      KEYS("F17", KEY_F17), KEYS("F18", KEY_F18), KEYS("F19", KEY_F19), KEYS("F20", KEY_F20),
      KEYS("F21", KEY_F21), KEYS("F22", KEY_F22), KEYS("F23", KEY_F23), KEYS("F24", KEY_F24),
    },
    KEYS("Esquerda", KEY_LEFT_ARROW),
    KEYS("Direita",  KEY_RIGHT_ARROW),
    KEYS("Enter",    KEY_RETURN),
  },
};
