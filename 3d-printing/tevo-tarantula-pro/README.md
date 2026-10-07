# Perfil PrusaSlicer – Tevo Tarantula Pro (BLTouch + Volcano)

Arquivo: `TevoTarantulaPro_PrusaSlicer_bundle.ini` (testado no PrusaSlicer 2.7).

## Como instalar

1. PrusaSlicer → **Arquivo › Importar › Importar Config Bundle…**
2. Selecione `TevoTarantulaPro_PrusaSlicer_bundle.ini`.
3. Escolha a impressora **Tevo Tarantula Pro - Volcano 0.4 BLTouch** (ou 0.6/0.8, conforme o bico).
   Os perfis de impressão aparecem filtrados automaticamente pelo diâmetro do bico.

## O que vem no pacote

| Impressora (bico Volcano) | Perfis de impressão |
|---|---|
| 0.4 mm | 0.12 DETAIL · 0.16 QUALITY · **0.20 NORMAL** · 0.24 FAST · 0.28 DRAFT |
| 0.6 mm | 0.20 DETAIL · **0.30 QUALITY** · 0.40 DRAFT |
| 0.8 mm | **0.40 QUALITY** · 0.55 DRAFT |

Filamentos: PLA, PLA+, PETG, ABS, TPU 95A (1,75 mm).

### Principais escolhas

- **Mesa** 235×235 mm, altura 250 mm, firmware `Marlin` (1.1.x, padrão da Tevo).
  Se você atualizou para Marlin 2.x, mude *Configurações da impressora › Geral › Tipo de G-code* para **Marlin 2**.
- **BLTouch**: o G-code inicial aquece a mesa, pré-aquece o bico a 150 °C (não escorre e não suja a sonda),
  faz `G28` + `G29` e ativa a compensação com `M420 S1 Z10`.
  Se preferir não sondar a cada impressão: rode `G29` uma vez, salve com `M500` e apague a linha `G29` do G-code inicial.
  O **Z-offset** fica no firmware (`M851 Z-x.xx` + `M500`) — o `z_offset` do fatiador está em 0.
- **Volcano**: a zona de fusão longa permite mais vazão; o limite está em cada filamento
  (`Velocidade volumétrica máx.`: PLA 15, ABS 14, PETG 10, TPU 2 mm³/s). Com bico 0.6/0.8 é aí que você ganha velocidade.
- **Retração (Bowden)**: 5 mm @ 40 mm/s com wipe e Z-hop 0,2 mm (bico 0.6: 4,5 mm; 0.8: 4 mm).
  PETG e TPU têm retração própria no perfil do filamento.
- **Velocidades/acelerações** conservadoras (perímetro externo 30 mm/s, preenchimento 70 mm/s, 500–1000 mm/s²)
  — o quadro da Tarantula Pro ganha mais qualidade com aceleração baixa do que com velocidade alta.
- Gerador de perímetros **Arachne**, preenchimento giroide 15 %, compensação de pé de elefante 0,15 mm.
- Os limites de máquina servem apenas para estimar o tempo; nada sobrescreve o firmware.

## Calibração recomendada (faça nesta ordem)

1. **E-steps** da extrusora (marque 120 mm no filamento, extrude 100 mm, ajuste `M92 E…` + `M500`).
2. **Z-offset** do BLTouch com a folha de papel na primeira camada.
3. **Torre de temperatura** para cada marca de filamento.
4. **Multiplicador de extrusão** (cubo de parede única) — ajuste em *Filamento › Multiplicador de extrusão*.
5. **Teste de retração** (comece em 5 mm e desça em passos de 0,5 mm).
6. Opcional: **Linear Advance** (`M900 K…`) se o seu firmware tiver — no Bowden fica tipicamente entre 0,3 e 0,8.
   Adicione no *G-code inicial do filamento*.

Depois de calibrar, salve cópias com seus valores (o ícone de disquete no PrusaSlicer) para não perder ao reimportar.
