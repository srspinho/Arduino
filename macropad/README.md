# MacroPad 4x3 + encoder (RP2350, Arduino IDE)

Macropad USB com **12 teclas mecânicas**, **encoder rotativo com clique**, **LCD colorido de 1.47"**
que mostra o que cada tecla faz, e **LEDs RGB** (opcionais). O PC reconhece como um teclado comum, sem driver nem
programa extra, no Windows, Linux e macOS.

![Pré-visualização da caixa](case/preview.png)

Telas geradas pelo próprio código do firmware, rodando num LCD simulado
(camada 1 com a tecla Play/Pause pressionada, camada 2 e camada 3):

<img src="firmware/lcd_telas.png" width="480" alt="Telas do LCD">

```
 ┌──────────────────────────────────┐
 │   [ LCD  Geral / Volume + ]   (◎)│  ← encoder
 │  ┌────┐ ┌────┐ ┌────┐ ┌────┐     │
 │  │ K1 │ │ K2 │ │ K3 │ │ K4 │     │
 │  └────┘ └────┘ └────┘ └────┘     │
 │  ┌────┐ ┌────┐ ┌────┐ ┌────┐     │
 │  │ K5 │ │ K6 │ │ K7 │ │ K8 │     │
 │  └────┘ └────┘ └────┘ └────┘     │
 │  ┌────┐ ┌────┐ ┌────┐ ┌────┐     │
 │  │ K9 │ │K10 │ │K11 │ │K12 │     │
 │  └────┘ └────┘ └────┘ └────┘     │
 └──────────────────────────────────┘
```

## Estrutura

| Pasta | Conteúdo |
|---|---|
| `firmware/MacroPad/` | Sketch do Arduino (`MacroPad.ino`). **Toda a personalização fica em `config.h`.** |
| `case/macropad_case.scad` | Caixa paramétrica em OpenSCAD |
| `case/stl/` | STLs prontos (versão para RP2350-Zero + LCD 1.47") |

---

## 1. Lista de materiais

| Qtd | Item | Observação |
|---|---|---|
| 1 | **Waveshare RP2350-Zero** *(ou Raspberry Pi Pico 2)* | USB nativo, o PC vê como teclado |
| 12 | Switches mecânicos MX (Gateron, Kailh, Outemu…) | Qualquer switch padrão MX |
| 12 | Keycaps MX 1u | Perfil baixo (DSA/XDA) fica bonito em macropad |
| 1 | Encoder **EC11** com botão (eixo D de 6 mm, rosca M7) | Com 20 ou 24 cliques por volta |
| 1 | LCD **1.47" 172x320 ST7789** (ex.: GMT147SPI) | Também dá para usar um OLED SSD1306 (veja a seção 3) |
| 12 | LEDs **WS2812B** *(opcional)* | Pedaço de fita 5 V ou módulos avulsos |
| 1 | Resistor 330 Ω *(se usar LEDs)* | Em série no fio de dados |
| 4 | Parafusos M3 x 8 mm cabeça panela | Rosqueiam direto no plástico |
| 4 | Pezinhos de borracha 8 mm | Opcional |
| — | Fio fino (AWG 26–28 ou fio de cabo de rede) | Para a ligação |

> Por que não o ESP32 comum? Ele **não tem USB nativo** e não aparece como
> teclado no PC (só por Bluetooth). A RP2350 faz isso direto e funciona muito bem
> com o Arduino IDE.

---

## 2. Ligação (sem matriz e sem diodos)

Cada tecla tem um pino próprio. Isso é mais simples de soldar e nunca dá
"ghosting". Os pinos escolhidos ficam nas **bordas da RP2350-Zero** e também
existem na **Pico 2**, então a mesma ligação serve para as duas.

| Componente | Pino do componente | Liga em |
|---|---|---|
| K1 … K12 | perna 1 | **GP0 … GP11** (K1=GP0, K2=GP1 … K12=GP11) |
| K1 … K12 | perna 2 | **GND** (todas juntas, um fio passando por todas) |
| Encoder | A | **GP26** |
| Encoder | B | **GP27** |
| Encoder | C (pino do meio, lado de 3 pinos) | **GND** |
| Encoder | SW (lado de 2 pinos) | **GP28** e o outro pino no **GND** |
| LCD | GND | **GND** |
| LCD | VCC | **3V3** |
| LCD | SCL (clock do SPI) | **GP14** |
| LCD | SDA (dado do SPI) | **GP15** |
| LCD | RES | **3V3** (fixo) |
| LCD | DC | **GP12** |
| LCD | CS | **GND** (fixo) |
| LCD | BL / "EL" (luz de fundo) | **GP13** (brilho por PWM) |
| LEDs WS2812B | 5V / GND | **5V** / **GND** |
| LEDs WS2812B | DIN | **GP29**, passando pelo resistor de 330 Ω |

> Os pinos **SCL/SDA do LCD são SPI**, apesar do nome parecido com I2C.
> **RES** e **CS** ficam fixos para economizar pinos, porque a RP2350-Zero tem só 20
> pinos nas bordas. Na **Pico 2** sobram pinos: se quiser, ligue RES e CS em GPIOs
> livres e coloque os números em `LCD_RST`/`LCD_CS` no `config.h`.

```
      RP2350-Zero (vista de cima, USB para cima)
                ___(USB-C)___
          5V  1|             |23 GP0  ── K1
         GND  2|             |22 GP1  ── K2
         3V3  3|             |21 GP2  ── K3
LEDs ── GP29  4|             |20 GP3  ── K4
Enc SW─ GP28  5|             |19 GP4  ── K5
Enc B ─ GP27  6|             |18 GP5  ── K6
Enc A ─ GP26  7|             |17 GP6  ── K7
LCD SDA GP15  8|             |16 GP7  ── K8
LCD SCL GP14  9|             |15 GP8  ── K9
               |_____________|
               GP13 GP12 GP11 GP10 GP9
                 BL   DC  K12  K11 K10
                (LCD)(LCD)
```

Não precisa de resistor nas teclas nem no encoder: o firmware liga os
pull-ups internos.

---

## 3. Arduino IDE

1. **Arquivo → Preferências → URLs adicionais para gerenciadores de placas**, cole:
   ```
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   ```
2. **Ferramentas → Placa → Gerenciador de Placas**: instale
   **"Raspberry Pi Pico/RP2040/RP2350"** (de Earle F. Philhower). Testado com a versão 6.3.2.
3. **Gerenciador de Bibliotecas**: instale
   - `Adafruit NeoPixel`
   - `Adafruit ST7735 and ST7789 Library` (ele pede também a `Adafruit GFX Library` e a `Adafruit BusIO`: aceite)
   - `Adafruit SSD1306`: só se for usar OLED
4. Abra `firmware/MacroPad/MacroPad.ino`.
5. **Ferramentas → Placa**: `Waveshare RP2350 Zero` (ou `Raspberry Pi Pico 2`).
   **Ferramentas → USB Stack**: deixe em `Pico SDK` (o padrão).
6. **Primeira gravação**: segure o botão **BOOT** da placa, conecte o USB e solte.
   A placa aparece como um pendrive. Clique em **Carregar**. Nas próximas vezes o
   Arduino IDE grava direto.

### Tela
No `config.h`, `DISPLAY_TYPE` escolhe a tela:

| Valor | Tela |
|---|---|
| `DISPLAY_LCD` *(padrão)* | LCD colorido 1.47" ST7789. Mostra a camada, as 12 teclas, o encoder e a última ação |
| `DISPLAY_OLED` | OLED SSD1306 0.91"/0.96" (I2C: SDA=GP12, SCL=GP13) |
| `DISPLAY_NONE` | Sem tela |

Se o LCD acender mas a imagem estiver estranha:
- **Cores invertidas** (fundo branco): mude `LCD_INVERT` para `0`.
- **Imagem de cabeça para baixo**: mude `LCD_ROTATION` para `3`.
- **Imagem deslocada ou com faixa de lixo na borda**: o seu módulo pode não ser
  172x320. Me avise o que aparece.
- **Tela toda preta**: confira o fio do BL (GP13) e se o VCC está em 3V3.

Sem LEDs: `USE_LEDS 0`.

---

## 4. Como usar

| Ação | O que faz |
|---|---|
| Apertar uma tecla | Executa a ação da camada atual (a tecla acende na tela) |
| Girar o encoder | Ação de girar da camada (padrão: **volume**) |
| Clicar no encoder | Ação de clique da camada (padrão: **mudo**) |
| **Segurar o encoder e girar** | **Troca de camada** (a cor da tela e dos LEDs muda) |
| Ficar 10 min sem usar | Apaga a tela e os LEDs. Qualquer tecla acorda |

### Camadas que já vêm configuradas

**1. Geral** (azul). Encoder: volume, e o clique deixa mudo.

| Copiar | Colar | Desfazer | Refazer |
|---|---|---|---|
| ⏮ Anterior | ⏯ Play/Pause | ⏭ Próxima | Calculadora |
| Print (Win+Shift+S) | Mostrar desktop | Gerenciador de Tarefas | Bloquear PC |

**2. Navegador** (laranja). Encoder: troca de aba, e o clique abre uma aba nova.

| Nova aba | Fechar aba | Reabrir aba | Atualizar (F5) |
|---|---|---|---|
| Voltar | Avançar | Zoom + | Zoom − |
| Janela anônima | Favoritar | Tela cheia | Zoom 100% |

**3. F13–F24** (roxo). Encoder: setas ←/→, e o clique dá Enter.
São teclas que nenhum teclado normal tem, ótimas para atalhos globais no
**OBS, Discord, Photoshop, DaVinci, AutoHotkey** etc. Elas não conflitam com nada.

---

## 5. Personalizar (`config.h`)

Cada tecla é uma linha:

```cpp
KEYS("Copiar",   KEY_LEFT_CTRL, 'c'),           // atalho (até 4 teclas)
MEDIA("Mudo",    KEY_MUTE),                     // tecla de mídia
TEXT("Email",    "meu.email@exemplo.com"),      // digita um texto
LAYER_NEXT,                                     // próxima camada
NONE,                                           // nada
```

- **Teclas especiais:** `KEY_LEFT_CTRL`, `KEY_LEFT_SHIFT`, `KEY_LEFT_ALT`,
  `KEY_LEFT_GUI` (tecla Windows), `KEY_F1`…`KEY_F24`, `KEY_RETURN`, `KEY_ESC`,
  `KEY_TAB`, setas `KEY_UP_ARROW` etc.
- **Mídia:** `KEY_PLAY_PAUSE`, `KEY_SCAN_NEXT`, `KEY_SCAN_PREVIOUS`, `KEY_MUTE`,
  `KEY_VOLUME_INCREMENT`, `KEY_VOLUME_DECREMENT`, `KEY_AL_CALCULATOR`,
  `KEY_BRIGHTNESS_INCREMENT`…
- **Mais camadas:** aumente `NUM_LAYERS` e copie um bloco `{ ... }`.
- **Cor de cada camada:** os três números depois do nome (R, G, B).
- **Nome na tela:** o primeiro texto de cada linha. Até ~8 letras cabem grandes;
  nomes com espaço quebram em 2 linhas ("Tela cheia"); nomes maiores saem com
  letra menor.
- **Encoder com o sentido trocado** ou contando 2 passos por clique: ajuste
  `ENC_REVERSE` e `ENC_STEPS_PER_DETENT`.

> ⚠️ **Teclado ABNT2:** o firmware envia teclas como um teclado americano.
> Atalhos com letras, números, F-keys e mídia funcionam normalmente. Mas
> `TEXT(...)` com símbolos (`@ / ? ; ç ~`) pode sair trocado se o Windows estiver
> em ABNT2. Use só letras e números no `TEXT`, ou configure o Windows em
> "Inglês (EUA) – Internacional".

---

## 6. Caixa impressa em 3D

Arquivos em `case/`. Os STLs prontos estão em `case/stl/`:

| Peça | Como imprimir |
|---|---|
| `macropad_bottom.stl` | Fundo na mesa, sem suporte. 3 paredes e 15–20% de preenchimento |
| `macropad_plate.stl` | Já está virado (topo na mesa), sem suporte. **Imprima com 100% de preenchimento** para os clipes dos switches ficarem firmes |
| `macropad_knob.stl` | Em pé, sem suporte |

Tamanho: **92 x 103 x 22 mm**. Camada de 0.2 mm em PLA ou PETG funciona bem.

**Mudar as opções** (abra o `.scad` no OpenSCAD e use o *Customizer*, ou edite no topo):

- `board = "pico2"` para a Raspberry Pi Pico 2 (furo micro-USB e berço maior)
- `screen = "oled091"` / `"oled096"` para usar OLED em vez do LCD (no `config.h`,
  `DISPLAY_TYPE DISPLAY_OLED` e `OLED_HEIGHT 32` ou `64`)
- `sw_cut = 14.1` se os switches entrarem muito justos
- `post_hole = 4.2` para usar insertos térmicos M3

> 📏 **Meça o seu LCD antes de imprimir o plate!** Os módulos de 1.47" variam
> de fabricante para fabricante. Os valores padrão são 44.5 x 22 mm de placa e
> área visível de 32.4 x 17.4 mm. Ajuste `lcd_pcb_w`, `lcd_pcb_h` e, se a área
> visível não estiver centralizada na plaquinha, `lcd_win_dx`. As 4 cantoneiras
> embaixo do plate só seguram os cantos, então os pinos podem sair por qualquer lado.

### Montagem
1. Encaixe os 12 switches no plate (de cima para baixo, até estalar).
2. Prenda o encoder com a porca que vem com ele.
3. Encaixe o LCD entre as 4 cantoneiras embaixo do plate, com a tela virada para a
   janela, e prenda com cola quente nas bordas ou fita dupla face. Tire a película
   de proteção antes.
4. Encaixe a placa no berço da caixa, com o USB virado para o furo de trás.
5. Solde os fios conforme a tabela da seção 2.
6. **Grave e teste antes de fechar.**
7. Feche com os 4 parafusos M3 e coloque o knob e as keycaps.

**LEDs:** numa montagem com fios soltos, o mais fácil é colar uma fita WS2812B
no fundo da caixa como *underglow* (`LED_PER_KEY 0`, e ajuste `LED_COUNT`). Para ter
um LED embaixo de cada tecla (`LED_PER_KEY 1`), o ideal é uma PCB própria, que
pode ser a versão 2 do projeto.
