#include "tela.h"
#include "config.h"
#include <Arduino_GFX_Library.h>

// O desenho e feito num "canvas" na RAM e enviado inteiro para o display,
// assim a tela nao pisca. (172 x 320 x 2 bytes = 110 KB)
static Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, GFX_NOT_DEFINED);
// O JD9853 da versao touch aceita os comandos padrao (MIPI DCS) do ST7789.
// Se a imagem ficar errada, veja a secao "Tela" do README.
static Arduino_GFX *display = new Arduino_ST7789(bus, LCD_RST, 0, LCD_IPS, LCD_LARGURA, LCD_ALTURA,
                                                  LCD_OFFSET_X, LCD_OFFSET_Y, LCD_OFFSET_X, LCD_OFFSET_Y);
static Arduino_Canvas *gfx = new Arduino_Canvas(LCD_LARGURA, LCD_ALTURA, display);

static const uint16_t COR_FUNDO    = RGB565_BLACK;
static const uint16_t COR_TEXTO    = RGB565_WHITE;
static const uint16_t COR_CINZA    = 0x8410;
static const uint16_t COR_VERDE    = 0x07E0;
static const uint16_t COR_AMARELO  = 0xFFE0;
static const uint16_t COR_LARANJA  = 0xFD20;
static const uint16_t COR_VERMELHO = 0xF800;

#if ESP_ARDUINO_VERSION_MAJOR < 3
static const int CANAL_BL = 0;
#endif

static const char *nomeCurto(uint8_t tipo) {
  switch (tipo) {
    case TIPO_RADAR_FIXO:         return "RADAR FIXO";
    case TIPO_SEMAFORO_RADAR:     return "SEMAF.+RADAR";
    case TIPO_RADAR_MOVEL:        return "RADAR MOVEL";
    case TIPO_SEMAFORO_CAMERA:    return "SEMAF.+CAM.";
    case TIPO_POLICIA_RODOVIARIA: return "POLICIA ROD.";
    case TIPO_PEDAGIO:            return "PEDAGIO";
    default:                      return "?";
  }
}

// A fonte padrao tem 6 x 8 pixels por caractere (multiplicado por "tamanho")
static void textoCentro(const char *txt, int y, uint8_t tamanho, uint16_t cor) {
  int largura = strlen(txt) * 6 * tamanho;
  gfx->setTextSize(tamanho);
  gfx->setTextColor(cor);
  gfx->setCursor((LCD_LARGURA - largura) / 2, y);
  gfx->print(txt);
}

bool telaIniciar() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(LCD_BL, 5000, 8);
#else
  ledcSetup(CANAL_BL, 5000, 8);
  ledcAttachPin(LCD_BL, CANAL_BL);
#endif
  telaBrilho(0);
  if (!gfx->begin()) return false;
  gfx->fillScreen(COR_FUNDO);
  gfx->flush();
  return true;
}

void telaBrilho(uint8_t nivel) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(LCD_BL, nivel);
#else
  ledcWrite(CANAL_BL, nivel);
#endif
}

void telaMensagem(const char *linha1, const char *linha2) {
  gfx->fillScreen(COR_FUNDO);
  textoCentro(linha1, 130, 2, COR_TEXTO);
  if (linha2) textoCentro(linha2, 160, 1, COR_CINZA);
  gfx->flush();
}

static void desenharPlaca(int cx, int cy, uint8_t limite) {
  gfx->fillCircle(cx, cy, 42, COR_VERMELHO);
  gfx->fillCircle(cx, cy, 33, RGB565_WHITE);
  char txt[8];
  if (limite > 0) snprintf(txt, sizeof(txt), "%u", limite);
  else strcpy(txt, "!");
  uint8_t tam = strlen(txt) >= 3 ? 3 : 4;
  int larg = strlen(txt) * 6 * tam;
  gfx->setTextSize(tam);
  gfx->setTextColor(RGB565_BLACK);
  gfx->setCursor(cx - larg / 2 + tam / 2, cy - 4 * tam + tam / 2);
  gfx->print(txt);
}

void telaDesenhar(const DadosTela &d) {
  const InfoAlvo *a = d.alvo;
  bool temAlvo = a && a->estado != ALERTA_NENHUM;
  bool excesso = temAlvo && a->estado == ALERTA_EXCESSO;
  // Em excesso o fundo pisca em vermelho (4 vezes por segundo)
  bool piscaVermelho = excesso && ((millis() / 250) % 2 == 0);

  gfx->fillScreen(piscaVermelho ? COR_VERMELHO : COR_FUNDO);
  char txt[24];

  // --- Barra de status ---
  gfx->setTextSize(2);
  gfx->setTextColor(d.gpsFix ? COR_VERDE : COR_LARANJA);
  gfx->setCursor(2, 2);
  snprintf(txt, sizeof(txt), "%dsat", d.satelites);
  gfx->print(txt);
  if (d.horaValida) {
    snprintf(txt, sizeof(txt), "%02d:%02d", d.hora, d.minuto);
    gfx->setTextColor(COR_TEXTO);
    gfx->setCursor(LCD_LARGURA - 5 * 12 - 2, 2);
    gfx->print(txt);
  }
  if (d.mudo) textoCentro("MUDO", 24, 1, piscaVermelho ? COR_TEXTO : COR_VERMELHO);
  gfx->drawFastHLine(0, 34, LCD_LARGURA, COR_CINZA);

  // --- Velocidade ---
  uint16_t corVel = COR_TEXTO;
  if (!d.gpsFix) {
    corVel = COR_CINZA;
  } else if (temAlvo && a->limiteKmh > 0) {
    if (excesso) corVel = piscaVermelho ? COR_TEXTO : COR_VERMELHO;
    else if (d.velKmh >= a->limiteKmh - MARGEM_AMARELO_KMH) corVel = COR_AMARELO;
  }
  if (d.gpsFix) snprintf(txt, sizeof(txt), "%d", (int)(d.velKmh + 0.5f));
  else strcpy(txt, "--");
  textoCentro(txt, 46, 9, corVel);
  textoCentro("km/h", 124, 2, piscaVermelho ? COR_TEXTO : COR_CINZA);
  gfx->drawFastHLine(0, 148, LCD_LARGURA, COR_CINZA);

  // --- Area do alerta ---
  if (!d.gpsFix) {
    textoCentro("BUSCANDO", 200, 2, COR_LARANJA);
    textoCentro("SATELITES...", 222, 2, COR_LARANJA);
    textoCentro("Deixe a antena com", 262, 1, COR_CINZA);
    textoCentro("vista para o ceu", 274, 1, COR_CINZA);
  } else if (!temAlvo) {
    textoCentro("VIA LIVRE", 215, 2, COR_VERDE);
    textoCentro("nenhum radar a frente", 245, 1, COR_CINZA);
  } else {
    desenharPlaca(LCD_LARGURA / 2, 198, a->limiteKmh);

    uint16_t corInfo = piscaVermelho ? COR_TEXTO : (excesso ? COR_VERMELHO : COR_AMARELO);
    textoCentro(excesso ? "REDUZA!" : nomeCurto(a->tipo), 248, 2, corInfo);

    if (a->distanciaM >= 1000) snprintf(txt, sizeof(txt), "%.1f km", a->distanciaM / 1000.0f);
    else snprintf(txt, sizeof(txt), "%d m", ((int)a->distanciaM / 10) * 10);
    textoCentro(txt, 270, 3, COR_TEXTO);

    // Barra que enche conforme o radar se aproxima
    int larguraBarra = LCD_LARGURA - 20;
    float frac = 1.0f - a->distanciaM / a->distanciaAlertaM;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    gfx->drawRect(10, 302, larguraBarra, 12, COR_TEXTO);
    gfx->fillRect(12, 304, (int)((larguraBarra - 4) * frac), 8, piscaVermelho ? COR_TEXTO : corInfo);
  }

  gfx->flush();
}
