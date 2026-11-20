# Oud Body Phase

**Oud Body Phase** é o primeiro plugin da suíte de processamento de áudio dedicada ao **Oud**, desenvolvido em **C++17**, **JUCE** e **CMake** para macOS (**Audio Unit**, **VST3** e **Standalone**).

Projetado especificamente para os desafios acústicos e de captação do instrumento, o plugin reúne em uma cadeia enxuta e de baixa latência quatro etapas essenciais: **alinhamento de fase e tempo entre fontes**, **escultura de graves e ressonância da caixa**, **suavização dinâmica do ataque da risha** e **controle de ganho de saída**.

---

## Sumário

- [Visão Geral e Cadeia de Sinal](#visão-geral-e-cadeia-de-sinal)
- [Parâmetros do Plugin](#parâmetros-do-plugin)
- [Guia Prático de Uso](#guia-prático-de-uso)
- [Identidade Visual (Oud Visualizer Design System)](#identidade-visual-oud-visualizer-design-system)
- [Arquitetura e Detalhes de DSP](#arquitetura-e-detalhes-de-dsp)
- [Requisitos e Compilação (macOS)](#requisitos-e-compilação-macos)
- [Validação e Uso no Logic Pro](#validação-e-uso-no-logic-pro)
- [Estrutura do Projeto](#estrutura-do-projeto)
- [Próximos Passos (Roadmap)](#próximos-passos-roadmap)

---

## Visão Geral e Cadeia de Sinal

Ao gravar ou amplificar um Oud — especialmente combinando **microfone externo** e **captação interna (piezo/mic blend)** — surgem três problemas recorrentes: cancelamento de fase pelo atraso acústico entre fontes, sobra de ressonância na caixa de madeira (região médio-grave) e transientes excessivamente secos ou estalados produzidos pela *risha*.

O sinal percorre a seguinte cadeia interna em série:

```text
Entrada (Mono / Estéreo)
   │
   ▼
[ 1. Fase & Alinhamento ] ── Inversão 180° + Atraso Fino (0–15 ms) + Alvo (Ambos / Canal 1 / Canal 2)
   │
   ▼
[ 2. Corpo & Caixa ]      ── Filtro Passa-Alta (HPF) + Filtro Paramétrico de Caixa (Peak EQ, Q = 2.2)
   │
   ▼
[ 3. Risha ]              ── Modelador Dinâmico de Transientes (envelope rápido vs. lento)
   │
   ▼
[ 4. Saída ]              ── Ganho de Saída suavizado (-18 dB a +12 dB)
   │
   ▼
Saída (Mono / Estéreo)
```

---

## Parâmetros do Plugin

Todos os controles são gerenciados via `AudioProcessorValueTreeState` (APVTS), garantindo automação na DAW e salvamento/restauração de estado em XML.

| Seção | Controle | ID do Parâmetro | Faixa | Padrão | Função Musical |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Fase & Alinhamento** | Inverter Fase (180°) | `inverter_fase` | Ligado / Desligado | Desligado | Inverte a polaridade do canal-alvo para corrigir cancelamento de graves/médios. |
| **Fase & Alinhamento** | Alvo | `alvo_fase` | `Ambos`, `Canal 1`, `Canal 2` | `Ambos` | Define qual canal recebe a inversão de polaridade e o atraso fino. |
| **Fase & Alinhamento** | Atraso Fino | `atraso_fino_ms` | `0.00 ms` a `15.00 ms` | `0.00 ms` | Atraso com interpolação linear para alinhar o tempo de chegada entre microfone e captador. |
| **Corpo & Caixa** | HPF | `freq_corte_hp_hz` | `30.0 Hz` a `250.0 Hz` | `80.0 Hz` | Filtro passa-alta (`Q = 0.707`) para remover subgraves indesejados, ruído de sala e batidas no tampo. |
| **Corpo & Caixa** | Freq Caixa | `freq_caixa_hz` | `140.0 Hz` a `520.0 Hz` | `240.0 Hz` | Seleciona a frequência central da ressonância principal do bojo do Oud. |
| **Corpo & Caixa** | Ganho Caixa | `ganho_caixa_db` | `-12.0 dB` a `+6.0 dB` | `0.0 dB` | Atenua o "sobrado" encaixotado ou devolve corpo acústico à madeira (`Q = 2.2`). |
| **Risha** | Suavizar Risha | `suavizar_risha` | `0.0 %` a `100.0 %` | `0.0 %` | Controla a intensidade da atenuação seletiva de transientes rápidos da palhetada. |
| **Saída** | Ganho Saída | `ganho_saida_db` | `-18.0 dB` a `+12.0 dB` | `0.0 dB` | Compensa variações de nível após a equalização e o controle de transientes. |

---

## Guia Prático de Uso

### Cenário 1: Alinhamento de Microfone Externo + Captação Interna (Faixa Estéreo / Dual-Mono)

Quando o Oud é gravado simultaneamente com um microfone externo e um captador interno, o sinal elétrico do captador chega à interface alguns milissegundos antes do som acústico atingir a cápsula do microfone.

1. Configure uma pista **Estéreo** (ou um barramento estéreo somando as duas fontes) onde, por exemplo, o **Canal 1 (L)** recebe o microfone externo e o **Canal 2 (R)** recebe a captação interna.
2. Insira o **Oud Body Phase** no *slot* de áudio e ajuste **Alvo** para **Canal 2** (o sinal mais adiantado).
3. Enquanto toca ou reproduz uma frase grave (como a corda solta mais baixa), teste primeiro **Inverter Fase (180°)** para identificar em qual polaridade os graves soam mais cheios e centrados.
4. Suba lentamente **Atraso Fino** entre `0.5 ms` e `3.5 ms` (referência aproximada: `1 ms` equivale a cerca de `34 cm` de distância no ar) até sentir a focalização do corpo acústico e a redução de timbres "ocos" (*comb filtering*).
5. Em seguida, esculpa os graves em **Corpo & Caixa** e ajuste **Suavizar Risha** para arredondar cliques excessivos do captador piezo.

### Cenário 2: Fonte Única (Mono — Microfone ou Captação Interna)

1. Insira o plugin em uma faixa **Mono** com **Alvo** em **Ambos** e **Atraso Fino** em `0.00 ms`.
2. Ajuste o **HPF** entre `65 Hz` e `100 Hz` para limpar vibrações estruturais sem perder o peso da região grave do instrumento.
3. Para localizar uma ressonância incômoda da caixa, eleve temporariamente **Ganho Caixa** para `+4 dB` ou `+6 dB`, varra **Freq Caixa** (`140 Hz–520 Hz`) até a nota problemática sobressair e, então, reduza **Ganho Caixa** para entre `-2 dB` e `-6 dB`.
4. Use **Suavizar Risha** entre `15%` e `45%` para domar ataques agressivos da palheta sem tirar a definição rítmica do *taqsim* ou das frases rápidas.
5. Equalize o volume final com **Ganho Saída** para manter a comparação justa com o *bypass*.

---

## Identidade Visual (Oud Visualizer Design System)

A interface gráfica (`800 × 336 px`) foi construída em `Source/OudLookAndFeel.h` seguindo diretamente o sistema visual do aplicativo **Oud Visualizer (v3.1)**, sob a premissa de contenção cromática (**Ébano/Pergaminho, Latão & Ameixa — Zero Ciano**):

- **Tema Ébano (Escuro — Padrão):** Superfícies em madeira escura (`#100d0b`, `#181310`, `#201a16`), texto em pergaminho claro (`#efe6d8`), acentos principais em **Latão** (`#c99a49` / `#e2b963`) e seção **Risha** destacada em **Ameixa** (`#c49be0`).
- **Tema Nahar (Claro — Pergaminho):** Alternável em tempo real pelo botão **Nahar / Ébano** no canto superior direito do cabeçalho, usando fundo em pergaminho iluminado (`#f4eee4` / `#fcfaf6`), latão escurecido (`#8f5e1c`) e ameixa profundo (`#6b3f8c`).
- **Tipografia:** *Cormorant Garamond* para títulos editoriais e *Plus Jakarta Sans* para rótulos e controles (com *fallback* automático para fontes nativas do macOS caso não estejam instaladas no sistema).

---

## Arquitetura e Detalhes de DSP

O motor de áudio foi escrito com foco em segurança em tempo real (*real-time safe*), evitando alocações dinâmicas dentro do `processBlock`:

- **`AlinhadorFase` (`Source/OudDspChain.h`):**
  - Utiliza `juce::dsp::DelayLine` com interpolação linear (capacidade interna até `25 ms`) e suavização (`juce::SmoothedValue`) tanto no atraso (`30 ms`) quanto na polaridade (`15 ms`) para eliminar cliques ao alternar a fase.
  - Canais fora do **Alvo** selecionado continuam alimentando a linha de atraso com leitura em `0 ms` para manter os buffers coerentes caso o usuário alterne o canal durante a reprodução.
- **`EscultorCaixa` (`Source/OudDspChain.h`):**
  - Combina um filtro passa-alta (`makeHighPass`, `Q = 0.707`) e um filtro de pico (`makePeakFilter`, `Q = 2.2`) por canal.
  - Os coeficientes IIR são recalculados **uma única vez por bloco de áudio** (`atualizarPorBloco`), avançando os suavizadores (`SmoothedValue::skip`) de acordo com o tamanho do buffer — reduzindo drasticamente o consumo de CPU em 48 kHz e 96 kHz.
- **`ModeladorRisha` (`Source/OudDspChain.h`):**
  - Detector de transientes de duplo envelope por canal: compara um envelope rápido (`ataque 0.5 ms`, `soltura 8 ms`) com um envelope lento (`ataque 6 ms`, `soltura 35 ms`).
  - Quando a razão entre os envelopes ultrapassa o limiar (`1.15`), aplica redução suave e proporcional ao parâmetro **Suavizar Risha**, preservando a sustentação natural da madeira.

---

## Requisitos e Compilação (macOS)

### Pré-requisitos

- **macOS** (testado em Apple Silicon / `arm64`)
- **Xcode** instalado com Command Line Tools
- **CMake** `>= 3.22`
- **JUCE** (v9.0.6 ou compatível) localizado em `~/JUCE` ou apontado pela variável de ambiente `JUCE_DIR`

### Gerar o projeto Xcode e compilar

```bash
cd ~/OudBodyPhase
cmake -B build -G Xcode
cmake --build build --target OudBodyPhase_AU OudBodyPhase_VST3 OudBodyPhase_Standalone --config Debug
```

Para uso definitivo na DAW com máxima otimização de CPU, compile em **Release**:

```bash
cmake --build build --target OudBodyPhase_AU OudBodyPhase_VST3 OudBodyPhase_Standalone --config Release
```

Com `COPY_PLUGIN_AFTER_BUILD TRUE` ativo no `CMakeLists.txt`, o CMake instala automaticamente os bundles após o build em:

- **Audio Unit (`.component`):** `~/Library/Audio/Plug-Ins/Components/Oud Body Phase.component`
- **VST3 (`.vst3`):** `~/Library/Audio/Plug-Ins/VST3/Oud Body Phase.vst3`
- **Standalone (`.app`):** `build/OudBodyPhase_artefacts/<Config>/Standalone/Oud Body Phase.app`

---

## Validação e Uso no Logic Pro

### 1. Validar o Audio Unit via `auval`

O plugin está registrado com os identificadores:
- **Tipo:** `aufx` (Audio Effect)
- **Subtipo (`PLUGIN_CODE`):** `Obp1`
- **Fabricante (`PLUGIN_MANUFACTURER_CODE`):** `OudL` (**OudAudioLab**)

Execute no terminal para validar:

```bash
auval -v aufx Obp1 OudL
```

O resultado esperado ao final do relatório é `AU VALIDATION SUCCEEDED`.

### 2. Carregar no Logic Pro

1. Caso tenha acabado de recompilar com o Logic Pro aberto, feche a DAW e limpe o registro de Audio Units:
   ```bash
   killall -9 AudioComponentRegistrar 2>/dev/null || true
   ```
2. Abra o Logic Pro e acesse **Logic Pro > Settings > Plug-in Manager**.
3. Localize **OudAudioLab > Oud Body Phase** e confirme o status **successfully validated** (se necessário, clique em **Reset & Rescan Selection**).
4. Insira o plugin no canal em **Audio FX > Audio Units > OudAudioLab > Oud Body Phase**.

---

## Estrutura do Projeto

```text
OudBodyPhase/
├── CMakeLists.txt             # Configuração CMake + JUCE (AU, VST3, Standalone)
├── README.md                  # Documentação do projeto
├── .gitignore                 # Exclusão de build/, caches Xcode e artefatos macOS
└── Source/
    ├── OudParameters.h        # IDs e layout APVTS dos 8 parâmetros
    ├── OudDspChain.h          # AlinhadorFase, EscultorCaixa, ModeladorRisha e CadeiaDspOud
    ├── OudLookAndFeel.h       # Design system Ébano / Nahar (Latão & Ameixa)
    ├── PluginProcessor.h      # Declaração do AudioProcessor e APVTS
    ├── PluginProcessor.cpp    # Ciclo de vida DSP, buses Mono/Estéreo e serialização XML
    ├── PluginEditor.h         # Declaração da janela, cartões e attachments APVTS
    └── PluginEditor.cpp       # Layout em 4 cartões, rótulos e alternância de tema
```

---

## Próximos Passos (Roadmap)

- [ ] Medidores visuais *thread-safe* (`std::atomic` + `juce::Timer`) para redução de transiente da **Risha** e pico de **Saída**
- [ ] Janela redimensionável com proporção travada
- [ ] Presets de fábrica para diferentes combinações de microfonação e captação interna
- [ ] Expansão da suíte **OudAudioLab** (ex.: afinador microtonal / *Maqam Tuner*)

---

## Licença

Defina a licença desejada para o repositório (ex.: MIT ou GPLv3, observando os termos de licenciamento do framework **JUCE** para distribuição pública).
