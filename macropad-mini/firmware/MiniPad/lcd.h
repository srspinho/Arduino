// =====================================================================
//  Envio dos quadros para o LCD ST7735S (roda no nucleo 1)
//
//  A biblioteca Adafruit_ST7735 cuida da inicializacao (cada versao do
//  modulo tem seus ajustes). Os pixels vao por DMA: enquanto um quadro
//  e enviado, o processador ja desenha o proximo no outro buffer.
// =====================================================================
#pragma once
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <hardware/dma.h>
#include <hardware/spi.h>

static Adafruit_ST7735 tft(&SPI1, LCD_CS, LCD_DC, LCD_RST);

static uint16_t frameBuf[2][LCD_W * LCD_H];   // 2 x 40 KB (o RP2040 tem 264 KB)
static uint8_t drawIdx = 0;                   // buffer onde o proximo quadro e desenhado
static int dmaCh = -1;
static bool dmaBusy = false;
static uint8_t blLevel = 0;

static void lcdBacklight(uint8_t level) {
  blLevel = level;
  analogWrite(LCD_BL, LCD_BL_ACTIVE_LOW ? 255 - level : level);
}

// Espera o envio anterior terminar e devolve o SPI para 8 bits (comandos)
static void lcdWaitDone() {
  if (!dmaBusy) return;
  dma_channel_wait_for_finish_blocking(dmaCh);
  while (spi_is_busy(spi1)) tight_loop_contents();
  while (spi_is_readable(spi1)) (void)spi_get_hw(spi1)->dr;   // descarta o que "voltou"
  spi_get_hw(spi1)->icr = SPI_SSPICR_RORIC_BITS;
  spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  tft.endWrite();
  dmaBusy = false;
}

void lcdBegin() {
  pinMode(LCD_BL, OUTPUT);
  analogWriteFreq(20000);   // PWM fora da faixa audivel e sem cintilar
  lcdBacklight(0);
  SPI1.setSCK(LCD_SCK);
  SPI1.setTX(LCD_MOSI);
  tft.initR(LCD_INIT);
  tft.setSPISpeed(LCD_SPI_HZ);
  tft.setRotation(LCD_ROTATION);
  tft.invertDisplay(LCD_INVERT);
  tft.fillScreen(ST77XX_BLACK);
#if LCD_USE_DMA
  dmaCh = dma_claim_unused_channel(true);
#endif
}

uint16_t *lcdDrawBuffer() { return frameBuf[drawIdx]; }

// Envia o buffer desenhado e troca para o outro
void lcdPush() {
  uint16_t *buf = frameBuf[drawIdx];
  lcdWaitDone();
  tft.startWrite();
  tft.setAddrWindow(0, 0, LCD_W, LCD_H);
#if LCD_USE_DMA
  // 16 bits por transferencia: cada pixel RGB565 sai inteiro, na ordem certa
  spi_set_format(spi1, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
  dma_channel_config c = dma_channel_get_default_config(dmaCh);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, spi_get_dreq(spi1, true));
  dma_channel_configure(dmaCh, &c, &spi_get_hw(spi1)->dr, buf, LCD_W * LCD_H, true);
  dmaBusy = true;
  drawIdx ^= 1;   // o proximo quadro e desenhado no outro buffer
#else
  tft.writePixels(buf, LCD_W * LCD_H, true, false);
  tft.endWrite();
#endif
}
