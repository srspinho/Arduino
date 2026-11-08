# Medidor de consumo de energia (ESP32 + Home Assistant)

Mede a corrente das duas fases na entrada do apartamento (bifásico 127/220 V,
Enel SP), estima potência e energia, e calcula no Home Assistant o custo do
ciclo de faturamento e a projeção da conta.

```
 Quadro de entrada                     ESP32 (ESPHome)            Home Assistant
 ┌───────────────┐   SCT-013 x2   ┌──────────────────────┐  Wi-Fi  ┌──────────────────┐
 │ Fase A ──(◯)──┼───────────────►│ GPIO34  corrente A   │────────►│ Painel Energia   │
 │ Fase B ──(◯)──┼───────────────►│ GPIO35  corrente B   │         │ Custo do ciclo   │
 │ Neutro        │                │ potência, kWh        │         │ Projeção da conta│
 └───────────────┘                └──────────────────────┘         └──────────────────┘
```

## Arquivos

| Arquivo | O que é |
|---|---|
| `esphome/medidor-energia.yaml` | Firmware do ESP32 (ESPHome) |
| `esphome/secrets.example.yaml` | Modelo de senhas. Copie para `secrets.yaml` |
| `homeassistant/packages/medidor_energia.yaml` | Tarifa, bandeira, ciclo, custo e projeção |
| `LISTA-DE-COMPRAS.md` | Componentes, links e preços estimados |

## 1. Como funciona a rede 127/220 V (e por que isso importa)

No bifásico da Enel SP chegam **duas fases + neutro**:

- **Fase ↔ neutro = 127 V**: tomadas, iluminação, geladeira etc.
- **Fase ↔ fase = 220 V**: chuveiro, ar-condicionado, forno elétrico.

As duas fases estão defasadas de **120°**, não de 180°, por isso
127 + 127 ≠ 220 (é 127 × √3 ≈ 220).

Consequência: o chuveiro de 220 V puxa a mesma corrente `I` nas duas fases,
mas consome `220 × I`, e não `127 × I + 127 × I = 254 × I`. Somar as fases
de forma ingênua **superestimaria o chuveiro em ~15%**.

Por isso o firmware separa a corrente em duas partes:

```
i_220 = min(Ia, Ib)                       → tratado como carga 220 V
P     = 220·i_220 + 127·(Ia − i_220) + 127·(Ib − i_220)
```

É uma heurística, e funciona bem quando há uma carga 220 V grande ligada
(o chuveiro, que costuma ser o maior consumo da casa). Cargas 127 V
equilibradas nas duas fases são contadas um pouco a menos. A calibração
contra o relógio da Enel (seção 6) absorve esse erro residual.

**Limitação da opção A:** sem medir tensão, o firmware calcula potência
*aparente* com tensão nominal fixa. Oscilações de tensão e o fator de
potência (motores, fontes) introduzem alguns % de erro, que também são
corrigidos pela calibração.

## 2. Materiais

Lista com links de compra e preços estimados: [LISTA-DE-COMPRAS.md](LISTA-DE-COMPRAS.md).

| Qtd | Item | Observação |
|---|---|---|
| 1 | ESP32 DevKit (esp32dev) | Qualquer placa com GPIO34/35 |
| 2 | **SCT-013-000** (100 A : 50 mA) | Saída em corrente, sem resistor interno |
| 2 | Resistor de burden **33 Ω** (1%) | Use **22 Ω** se o disjuntor geral for maior que 63 A |
| 2 | Resistor 10 kΩ | Divisor de polarização |
| 1 | Capacitor eletrolítico 10 µF | Estabiliza a polarização |
| 2 | Conector P2 fêmea (3,5 mm) | Para o plugue do sensor |
| 1 | Fonte USB 5 V | Pode ser um carregador de celular numa tomada próxima |
| 1 | Caixa plástica | Montagem fora dos barramentos |

**Por que 33 Ω?** Com 63 A, o secundário tem 31,5 mA RMS, ou 44,5 mA de pico.
No resistor de 33 Ω isso dá ±1,47 V em torno de 1,65 V, o que cabe na
faixa útil do ADC (~0,15 a 3,1 V). Com 22 Ω o limite sobe para ~100 A, ao
custo de menos resolução.

## 3. Esquema

A polarização (Vbias ≈ 1,65 V) é compartilhada pelos dois sensores:

```
 3V3 ──[10kΩ]──┬──[10kΩ]── GND
               │
               ├──(+)[10µF](−)── GND
               │
             Vbias ─────────────┬────────────────────┐
                                │                    │
              SCT-013 fase A    │     SCT-013 fase B │
              ┌───────────┐     │     ┌───────────┐  │
              │ ponta capa│     │     │ ponta capa│  │
              └──┬─────┬──┘     │     └──┬─────┬──┘  │
                 ├[33Ω]┤        │        ├[33Ω]┤     │
                 │     └────────┘        │     └─────┘
     GPIO34 ◄────┘           GPIO35 ◄────┘
```

- O burden fica **em paralelo** com o sensor, soldado direto no conector P2.
  Assim o secundário nunca fica aberto.
- Use **somente ADC1 (GPIO32–39)**. O ADC2 deixa de funcionar quando o Wi-Fi está ligado.
- Mantenha os fios dos sensores longe da fonte e do ESP32 para reduzir ruído.

## 4. Instalação no quadro ⚠️

1. **Desligue o disjuntor do medidor** (no hall / prumada) ou o geral do
   apartamento antes de abrir o quadro. Se não se sentir seguro, chame um
   eletricista.
2. Identifique os **dois fios de fase** que chegam no disjuntor geral bipolar.
3. Abrace **cada fase com um sensor**, nunca o neutro e nunca dois fios
   juntos. A seta do sensor deve apontar no mesmo sentido nos dois
   (rede → apartamento).
4. Passe os cabos dos sensores para fora do quadro até a caixa do ESP32.
   Não deixe nada encostado em barramentos.
5. Feche o quadro e religue.

> Nunca desconecte o plugue do SCT-013-000 do circuito com corrente
> passando se o burden **não** estiver soldado no próprio conector. O
> secundário aberto gera alta tensão.

## 5. Firmware (ESPHome)

```bash
pip install esphome
```

```bash
cd projetos/medidor-energia/esphome
```

```bash
cp secrets.example.yaml secrets.yaml
```

Edite `secrets.yaml` (Wi-Fi e chave da API) e grave o firmware pelo USB na
primeira vez:

```bash
esphome run medidor-energia.yaml
```

Depois disso as atualizações vão por Wi-Fi (OTA). O Home Assistant detecta o
dispositivo sozinho em **Configurações → Dispositivos → ESPHome**. Ele pede a
mesma `api_encryption_key` do `secrets.yaml`.

Se preferir, cole o YAML no add-on **ESPHome Device Builder** do Home Assistant.

## 6. Calibração

### 6.1 Ganho de cada sensor: use o chuveiro

O chuveiro é uma carga resistiva de 220 V com potência conhecida, o que o
torna uma referência ótima.

1. Desligue o que der na casa e anote `Corrente Fase A` e `Fase B` (base).
2. Ligue o chuveiro na posição de maior potência (ex.: 7500 W) e espere ~30 s.
3. Corrente esperada: `I = P / 220` (7500 W → **34,1 A**).
4. O aumento em cada fase deve ser esse valor. Se a fase A subiu 31,0 A:

   ```
   ganho_fase_a_novo = 60.6 × 34.1 / 31.0 = 66.7
   ```
5. Ajuste `ganho_fase_a` / `ganho_fase_b` nas `substitutions` e regrave.

Se tiver um alicate amperímetro, compare direto com ele, que é ainda melhor.

### 6.2 Fator global: compare com o relógio da Enel

1. Anote a leitura do relógio da Enel e o `Energia Total` do medidor.
2. Depois de 7 a 15 dias, anote de novo.
3. `fator_correcao = (Δ relógio Enel) / (Δ medidor)`
4. Ajuste `fator_correcao` e regrave.

## 7. Home Assistant: custo e projeção

1. Habilite packages no `configuration.yaml`:

   ```yaml
   homeassistant:
     packages: !include_dir_named packages
   ```
2. Copie `homeassistant/packages/medidor_energia.yaml` para `<config>/packages/`.
3. Confira o `entity_id` do sensor `Energia Total`
   (padrão: `sensor.medidor_de_energia_energia_total`) e o **dia de leitura**
   no `cron` do `utility_meter`. Os dois estão no topo do arquivo.
4. Reinicie o HA e preencha uma vez (os valores ficam salvos):

| Entidade | Como obter | Exemplo |
|---|---|---|
| Tarifa efetiva | `(total da conta − CIP − bandeira) ÷ kWh faturados` | 0,95 R$/kWh |
| Fator de impostos | ~1,32 (ICMS 18% + PIS/COFINS ~6%) | 1,32 |
| Taxas fixas (CIP) | Linha "Contrib. Ilum. Pública" da conta | 15,00 R$ |
| Consumo mínimo | Bifásico = **50 kWh** (custo de disponibilidade) | 50 |
| Bandeira | Muda todo mês, veja em aneel.gov.br | Amarela |

Os adicionais de bandeira no pacote são os **valores ANEEL de 2025**
(amarela R$ 1,885, vermelha 1 R$ 4,463 e vermelha 2 R$ 7,877 por 100 kWh,
sem impostos). Confira se mudaram.

### Sensores criados

| Sensor | Descrição |
|---|---|
| `sensor.energia_consumo_do_ciclo` | kWh desde a última leitura da Enel |
| `sensor.energia_preco_do_kwh` | Tarifa + bandeira (com impostos) |
| `sensor.energia_custo_do_ciclo` | Valor acumulado da conta até agora |
| `sensor.energia_projecao_do_ciclo` | kWh projetado para 30 dias |
| `sensor.energia_projecao_da_conta` | **Valor estimado da próxima conta** |

### Painel Energia nativo

Em **Configurações → Painéis → Energia → Consumo da rede**, adicione
`Energia Total` e escolha *"Usar uma entidade com o preço atual"* →
`sensor.energia_preco_do_kwh`. O painel mostra o consumo e o custo por
hora, dia e mês. Ele não considera o mínimo de 50 kWh nem a CIP; para isso
use o `Custo do ciclo`.

## 8. Próximos passos possíveis

- **Medir a tensão** com um transformador AC-AC 9 V numa tomada. Isso dá a
  potência ativa real e o fator de potência, e elimina a heurística 127/220
  (opção B).
- **PZEM-004T v3** em cada fase, para precisão de ~0,5% (opção C).
- Alertas: chuveiro ligado por mais de 20 min, consumo acima da média etc.
- ADC externo **ADS1115** se o ruído do ADC interno incomodar em correntes baixas.
