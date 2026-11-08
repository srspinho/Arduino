# Lista de compras - Medidor de energia

Os links abrem **buscas** (anúncios específicos somem com frequência).
Os preços são **estimativas** de referência (nov/2026). Confira na hora de comprar.

## Essenciais

| Qtd | Item | O que procurar no anúncio | Preço estimado (un.) | Mercado Livre | Shopee | AliExpress |
|---|---|---|---|---|---|---|
| 2 | Sensor de corrente **SCT-013-000** | "100A **50mA**" (NÃO 30A/1V, 50A/1V etc.), com plugue P2 | R$ 35–60 (BR) / R$ 15–25 (AliExpress) | [buscar](https://lista.mercadolivre.com.br/sct-013-000-100a) | [buscar](https://shopee.com.br/search?keyword=sct-013-000%20100a) | [buscar](https://pt.aliexpress.com/w/wholesale-sct013-000-100a.html) |
| 1 | **ESP32 DevKit V1** (30 pinos) | ESP32-WROOM-32, USB **CP2102** ou **CH340**, pinos soldados, de preferência USB-C | R$ 35–75 | [buscar](https://lista.mercadolivre.com.br/esp32-devkit-v1-30-pinos) | [buscar](https://shopee.com.br/search?keyword=esp32%20devkit%20v1%2030%20pinos) | [buscar](https://pt.aliexpress.com/w/wholesale-esp32-devkit-30pin.html) |
| 2 | Resistor **33 Ω** 1% ¼ W (burden) | Filme metálico 1%. Use **22 Ω** se o disjuntor geral for > 63 A | R$ 0,20–1 | [buscar](https://lista.mercadolivre.com.br/resistor-33r-1%25-filme-metalico) | [buscar](https://shopee.com.br/search?keyword=resistor%2033%20ohm%201%25) | — |
| 2 | Resistor **10 kΩ** ¼ W | Qualquer resistor de 10 kΩ, 1% ou 5% | R$ 0,10–0,50 | [buscar](https://lista.mercadolivre.com.br/resistor-10k-1-4w) | [buscar](https://shopee.com.br/search?keyword=resistor%2010k) | — |
| 1 | Capacitor eletrolítico **10 µF** (≥ 16 V) | Qualquer marca | R$ 0,30–1 | [buscar](https://lista.mercadolivre.com.br/capacitor-eletrolitico-10uf-25v) | [buscar](https://shopee.com.br/search?keyword=capacitor%20eletrolitico%2010uf) | — |
| 2 | Conector **P2 fêmea 3,5 mm** para placa ou painel | Estéreo, de painel (rosca) ou de PCI | R$ 2–5 | [buscar](https://lista.mercadolivre.com.br/jack-p2-femea-painel-estereo) | [buscar](https://shopee.com.br/search?keyword=jack%20p2%20femea%20painel) | — |
| 1 | Placa perfurada (ilhada) ~5×7 cm | Fenolite ou fibra | R$ 5–15 | [buscar](https://lista.mercadolivre.com.br/placa-perfurada-ilhada-5x7) | [buscar](https://shopee.com.br/search?keyword=placa%20perfurada%205x7) | — |
| 1 | Barra de pinos fêmea (soquete para o ESP32) | 2 tiras de 15 pinos, para não soldar o ESP32 direto | R$ 3–8 | [buscar](https://lista.mercadolivre.com.br/barra-de-pinos-femea-180) | [buscar](https://shopee.com.br/search?keyword=barra%20de%20pinos%20femea) | — |
| 1 | Caixa plástica (patola) ~10×7×4 cm | Que caiba a placa e tenha espaço para os conectores | R$ 10–25 | [buscar](https://lista.mercadolivre.com.br/caixa-patola-plastica) | [buscar](https://shopee.com.br/search?keyword=caixa%20patola) | — |
| 1 | Fonte USB 5 V 1 A + cabo USB | Pode ser um carregador de celular que você já tenha | R$ 0–30 | [buscar](https://lista.mercadolivre.com.br/fonte-carregador-usb-5v-1a) | — | — |

**Total estimado: ~R$ 120–230** (menos se você já tiver fonte, resistores e caixa).

> **Dica:** um **kit de resistores** (600 peças, 30 valores) custa ~R$ 20–35 e
> traz 10 kΩ e 33 Ω (às vezes 22 Ω). Vale a pena se você mexe com Arduino:
> [Mercado Livre](https://lista.mercadolivre.com.br/kit-resistores-600-pecas-1%25) ·
> [Shopee](https://shopee.com.br/search?keyword=kit%20resistores%20600%20pecas)

## Ferramentas (se ainda não tiver)

| Item | Para quê | Preço estimado | Link |
|---|---|---|---|
| Ferro de solda 40–60 W + estanho | Montar a placa | R$ 40–100 | [buscar](https://lista.mercadolivre.com.br/ferro-de-solda-60w-estanho) |
| Multímetro | Conferir a montagem e a tensão de polarização (~1,65 V) | R$ 40–80 | [buscar](https://lista.mercadolivre.com.br/multimetro-digital) |
| **Alicate amperímetro** (opcional) | Calibração mais precisa dos sensores | R$ 80–200 | [buscar](https://lista.mercadolivre.com.br/alicate-amperimetro-digital) |

## Upgrades opcionais (futuro)

| Item | Para quê | Link |
|---|---|---|
| Transformador AC-AC 9 V (ou 12 V), de tomada | Medir a tensão e o fator de potência (opção B) | [buscar](https://lista.mercadolivre.com.br/transformador-9v-ac-ac) |
| Módulo **ADS1115** (ADC 16 bits) | Menos ruído em correntes baixas | [buscar](https://lista.mercadolivre.com.br/ads1115) |
| **PZEM-004T v3** 100 A (com TC), 1 por fase | Precisão de ~0,5% (opção C) | [buscar](https://lista.mercadolivre.com.br/pzem-004t-v3-100a) |

## Cuidados na compra

- **SCT-013: confira a saída.** Existem várias versões parecidas:
  - **SCT-013-000 = 100 A : 50 mA** (saída em corrente, **sem** resistor interno) ← **é este**
  - SCT-013-030 / 050 / 100 = saída **1 V**, com resistor interno. Também
    funcionam, mas **não** use o burden de 33 Ω e mude o `ganho_fase_*` no
    firmware para a corrente nominal (ex.: 30 A/1 V → `ganho = 30`).
- **Abertura da garra:** o SCT-013 aceita condutores de até ~13 mm. Cabos de
  entrada de 10–16 mm² cabem. Confira se há folga no quadro.
- **AliExpress:** é mais barato, mas o prazo é de 2 a 5 semanas e pode haver
  imposto de importação. Os componentes passivos (resistores, P2, caixa)
  saem mais em conta em lojas nacionais.
