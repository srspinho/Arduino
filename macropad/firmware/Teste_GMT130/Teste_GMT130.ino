// =====================================================================
//  TESTE MINIMO DO LCD GMT130 (1.3" 240x240, ST7789, sem pino CS)
//
//  Sem biblioteca e sem o periferico SPI: liga/desliga cada pino na mao,
//  no SPI modo 3 (clock parado em nivel alto), e pinta a memoria INTEIRA
//  do controlador (240x320) de uma cor de cada vez. Se a ligacao estiver
//  certa, a tela TEM que mudar de cor - nao depende de rotacao nem de
//  deslocamento.
//
//  Ligacao: GND->GND  VCC->3V3  SCL->GP14  SDA->GP15
//           RES->GP29  DC->GP12  BLK->GP13
//  Monitor Serial: 115200.  Placa: Waveshare RP2350 Zero.
// =====================================================================

#define PIN_SCL 14
#define PIN_SDA 15
#define PIN_RES 29
#define PIN_DC  12
#define PIN_BLK 13

// Um bit no modo 3: o clock desce, o dado muda, o clock sobe (o LCD le na subida)
static inline void sendByte(uint8_t b) {
  for (int i = 7; i >= 0; i--) {
    digitalWrite(PIN_SCL, LOW);
    digitalWrite(PIN_SDA, (b >> i) & 1);
    delayMicroseconds(1);
    digitalWrite(PIN_SCL, HIGH);
    delayMicroseconds(1);
  }
}

void command(uint8_t c) {
  digitalWrite(PIN_DC, LOW);
  sendByte(c);
  digitalWrite(PIN_DC, HIGH);
}

void data(uint8_t d) {
  sendByte(d);   // DC ja fica em HIGH depois de command()
}

void fillAll(uint16_t color) {
  command(0x2A);              // colunas 0..239
  data(0); data(0); data(0); data(239);
  command(0x2B);              // linhas 0..319 (memoria inteira)
  data(0); data(0); data(0x01); data(0x3F);
  command(0x2C);              // escrever na memoria
  uint8_t hi = color >> 8, lo = color & 0xFF;
  for (uint32_t i = 0; i < 240UL * 320UL; i++) {
    data(hi);
    data(lo);
  }
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);
  Serial.println("\n===== TESTE MINIMO GMT130 (bit-bang, SPI modo 3) =====");

  pinMode(PIN_SCL, OUTPUT);
  pinMode(PIN_SDA, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_RES, OUTPUT);
  pinMode(PIN_BLK, OUTPUT);
  digitalWrite(PIN_SCL, HIGH);   // modo 3: clock parado em nivel alto
  digitalWrite(PIN_DC, HIGH);
  digitalWrite(PIN_BLK, HIGH);   // luz de fundo

  Serial.println("Reset por hardware (RES no GP29)...");
  digitalWrite(PIN_RES, HIGH);
  delay(20);
  digitalWrite(PIN_RES, LOW);
  delay(20);
  digitalWrite(PIN_RES, HIGH);
  delay(150);

  Serial.println("Inicializando o ST7789...");
  command(0x01); delay(150);          // SWRESET
  command(0x11); delay(150);          // SLPOUT  (sai do modo dormir)
  command(0x3A); data(0x55); delay(10); // COLMOD: 16 bits por pixel
  command(0x36); data(0x00);          // MADCTL: orientacao padrao
  command(0x21);                      // INVON (os IPS precisam de inversao)
  command(0x13);                      // NORON
  command(0x29); delay(50);           // DISPON  (liga a imagem)
  Serial.println("Pronto. Agora a tela deve trocar de cor a cada ~2 s.");
}

void loop() {
  static const uint16_t colors[] = {0xF800, 0x07E0, 0x001F, 0xFFFF, 0x0000};
  static const char *names[] = {"VERMELHO", "VERDE", "AZUL", "BRANCO", "PRETO"};
  for (uint8_t i = 0; i < 5; i++) {
    Serial.printf("Pintando de %s... ", names[i]);
    uint32_t t = millis();
    fillAll(colors[i]);
    Serial.printf("ok (%lu ms)\n", (unsigned long)(millis() - t));
    delay(1500);
  }
}
