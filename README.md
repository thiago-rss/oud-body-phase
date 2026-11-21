# Oud Body Phase

Plugin de áudio (AU, VST3 e Standalone) desenvolvido em **C++17 / JUCE** como parte da suíte **OudAudioLab**, projetado especificamente para o alinhamento de fase entre microfone e captação interna (piezo/blend), escultura de ressonância de caixa acústica e controle suave de transientes da *risha* no Alaúde Árabe (Oud).

---

## Recursos Principais (`v0.3.0`)

### 1. Fase & Alinhamento (`AlinhadorFase` + `AnalisadorCorrelacaoFase`)
* **Inversão de Polaridade (180°):** suavizada (`15 ms`) para alternância sem cliques.
* **Atraso Fino com Interpolação `Lagrange3rd`:** linha de atraso cúbica de 3ª ordem (em vez de interpolação linear) para preservar o brilho e os harmônicos agudos das cordas em atrasos fracionários de amostra.
* **Latência Base Simétrica (2 amostras):** ambos os canais compartilham o mesmo *offset* base de 2 amostras (reportado ao host via `setLatencySamples`), garantindo atraso relativo exato de `0,00 ms` e estabilidade da interpolação `Lagrange3rd`.
* **Seletor de Alvo (`Ambos`, `Canal 1`, `Canal 2`):** permite alinhar ou inverter apenas a fonte interna contra o microfone em faixas estéreo/dual-mono.
* **Medidor de Correlação de Fase (`-1` a `+1`):** calculado imediatamente após o estágio de fase com integrador exponencial (`~120 ms`) em precisão `double` e limiar otimizado em **`-90 dBFS`** para responder até em passagens suaves do Oud (*Latão* para fase positiva, *Terracota* para cancelamento).

### 2. Corpo & Caixa (`EscultorCaixa`)
* **HPF (Passa-Altas):** corte limpo (`20–400 Hz`) para remover graves indesejados e ruído de manuseio.
* **Escultura de Caixa:** filtro *peak* em precisão `double` (`100–800 Hz`, `-18` a `+12 dB`, `Q = 2.2`) com atualização de coeficientes uma vez por bloco (sem alocações na *audio thread*) para domar o bojo excessivo ou encorpar o instrumento.

### 3. Modelador de *Risha* (`ModeladorRisha`)
* **Controle de Transiente:** detector de envelope duplo (rápido `0,5 ms / 8 ms` contra lento `6 ms / 35 ms`) que suaviza o estalo da palheta (*risha*) sem esmagar o corpo nem o decaimento acústico.
* **Medidor de Redução de Ganho (GR):** barra vertical descendente (`0` a `-12 dB`) em **Ameixa**, com balística suave e captura *lock-free* de picos curtos.

### 4. Saída & Medição (`MedidorPicoSaida`)
* **Ganho de Saída Suavizado:** controle de nível final livre de artefatos de zíper.
* **Medidor de Pico (`dBFS`):** barra vertical ascendente em **Latão** com escala expandida próxima a `0 dBFS`, marcador de *peak-hold* (`~1,15 s`) e indicador de *clip* acima de `-0.1 dBFS` em **Terracota** (clicável para limpar).

### 5. Interface Adaptativa e Identidade Visual
* **Adaptação Automática Mono / Estéreo:** monitora o barramento ativo de forma *lock-free* (`std::atomic<int>`). Em faixas **Mono**, oculta automaticamente o seletor **Alvo** e o medidor de **Correlação de Fase**, mantendo a interface limpa e aplicando fase/atraso diretamente no canal único.
* **Temas Ébano (escuro) e Nahar (claro):** identidade visual da linha *Oud Visualizer* baseada na paleta **Latão**, **Ameixa** e **Terracota**, em janela compacta de **`900 × 336 px`** com renderização UTF-8 limpa.

---

## Arquitetura *Thread-Safe* e DSP

Toda a comunicação entre a *audio thread* e a interface gráfica (`juce::Timer` a `30 Hz`) é estritamente *lock-free* e livre de alocações dinâmicas em `processBlock`:

* **`MedidorAtomico` (`std::atomic<float>`):** retém o maior pico ocorrido entre dois quadros da UI via `compare_exchange_weak` e entrega/zera com `exchange(0.0f)`, evitando perder ataques muito curtos da *risha* ou picos de saída.
* **`MedidorCorrelacaoAtomico`:** publica continuamente o valor bipolar (`-1.0` a `+1.0`) e o estado (`Silêncio`, `Mono`, `Ativo`) com semântica `acquire/release`.

---

## Estrutura do Projeto

```text
OudBodyPhase/
├── CMakeLists.txt
├── LICENSE
├── README.md
└── Source/
    ├── OudParameters.h     # IDs e layout de parâmetros (APVTS)
    ├── OudDspChain.h       # AlinhadorFase (Lagrange3rd), Correlação, EscultorCaixa, ModeladorRisha e atômicos
    ├── OudLookAndFeel.h    # Paletas Ébano/Nahar, tipografia e desenho dos controles
    ├── OudMeters.h         # MedidorReducaoRisha, MedidorPicoSaida e MedidorCorrelacaoFase
    ├── PluginProcessor.h
    ├── PluginProcessor.cpp # Processamento de áudio, estado XML e ponte lock-free
    ├── PluginEditor.h
    └── PluginEditor.cpp    # Layout adaptativo Mono/Estéreo e atualização a 30 Hz
