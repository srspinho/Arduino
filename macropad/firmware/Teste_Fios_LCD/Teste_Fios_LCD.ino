// =====================================================================
//  TESTE DOS FIOS DO LCD - com multimetro
//
//  Liga UM sinal do LCD de cada vez em 3,3 V (os outros ficam em 0 V),
//  trocando a cada 5 segundos. O Monitor Serial (115200) diz qual esta
//  ligado. Meca com o multimetro (escala de 20 V DC):
//    ponta PRETA  -> pino GND do LCD
//    ponta VERMELHA -> cada pino do LCD (SCL, SDA, RES, DC)
//  O pino anunciado deve marcar ~3,3 V e os outros ~0 V.
//  Meca NOS PINOS DO LCD (nao na placa): assim testa fio e solda juntos.
//
//  O LED da placa mostra a fase: vermelho=SCL, verde=SDA, azul=RES, amarelo=DC
//  Placa: Waveshare RP2350 Zero.
// =====================================================================
#include <Adafruit_NeoPixel.h>

#define LCD_BL 13

struct Signal {
  const char *lcdPin;
  uint8_t gpio;
  uint8_t r, g, b;
};

const Signal SIGNALS[] = {
  {"SCL", 14, 255, 0, 0},
  {"SDA", 15, 0, 255, 0},
  {"RES", 29, 0, 0, 255},
  {"DC",  12, 255, 160, 0},
};
const uint8_t N = sizeof(SIGNALS) / sizeof(SIGNALS[0]);

Adafruit_NeoPixel onboard(1, 16, NEO_GRB + NEO_KHZ800);

void setup() {
  onboard.begin();
  onboard.setBrightness(30);
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 5000) delay(10);

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  for (uint8_t i = 0; i < N; i++) {
    pinMode(SIGNALS[i].gpio, OUTPUT);
    digitalWrite(SIGNALS[i].gpio, LOW);
  }
  Serial.println("\n===== TESTE DOS FIOS DO LCD =====");
  Serial.println("Ponta preta no GND do LCD. Ponta vermelha em cada pino do LCD.");
  Serial.println("Antes de tudo: VDD do LCD deve marcar ~3,3 V e CS ~0 V o tempo todo.");
}

void loop() {
  for (uint8_t i = 0; i < N; i++) {
    for (uint8_t j = 0; j < N; j++) digitalWrite(SIGNALS[j].gpio, j == i ? HIGH : LOW);
    onboard.setPixelColor(0, onboard.Color(SIGNALS[i].r, SIGNALS[i].g, SIGNALS[i].b));
    onboard.show();
    Serial.printf("\nAGORA: so o GP%u esta em 3,3 V.\n", SIGNALS[i].gpio);
    Serial.printf("  -> o pino %s do LCD deve marcar ~3,3 V\n", SIGNALS[i].lcdPin);
    Serial.print("  -> estes devem marcar ~0 V: ");
    for (uint8_t j = 0; j < N; j++)
      if (j != i) Serial.printf("%s ", SIGNALS[j].lcdPin);
    Serial.println();
    delay(5000);
  }
}
