// =====================================================================
//  Fila de eventos entre os dois nucleos do RP2040
//
//  Nucleo 0: le teclas e encoder e fala com o PC pela USB (sempre rapido).
//  Nucleo 1: desenha a tela e o LED (as animacoes nunca atrasam a USB).
//  O nucleo 0 so escreve na fila e o nucleo 1 so le: nao precisa de trava.
// =====================================================================
#pragma once
#include <stdint.h>

enum EventType : uint8_t {
  EV_KEY_DOWN,    // a = tecla
  EV_KEY_UP,      // a = tecla
  EV_ENC_TURN,    // a = camada, b = cliques (+ direita / - esquerda)
  EV_ENC_CLICK,   // a = camada
  EV_LAYER,       // a = nova camada, b = direcao (+1 / -1)
  EV_ENC_HOLD,    // botao do encoder apertado (b = 1) ou solto (b = 0)
};

struct Event {
  EventType type;
  uint8_t a;
  int16_t b;
};

class EventQueue {
 public:
  bool push(const Event &e) {
    uint8_t h = head, n = (h + 1) % SIZE;
    if (n == tail) return false;   // cheia: descarta (so a tela perde o evento)
    buf[h] = e;
    __sync_synchronize();
    head = n;
    return true;
  }
  bool pop(Event &e) {
    uint8_t t = tail;
    if (t == head) return false;
    __sync_synchronize();
    e = buf[t];
    __sync_synchronize();
    tail = (t + 1) % SIZE;
    return true;
  }

 private:
  static const uint8_t SIZE = 64;
  Event buf[SIZE];
  volatile uint8_t head = 0, tail = 0;
};

static EventQueue events;

// O nucleo 1 avisa aqui quando a tela esta apagada (para WAKE_EATS_KEY)
static volatile bool uiSleeping = false;
