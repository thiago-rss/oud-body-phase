#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <vector>

enum class AlvoCanalFase : int
{
    Ambos  = 0,
    Canal1 = 1,
    Canal2 = 2
};

struct ParametrosDspOud
{
    bool          inverterFase  { false };
    float         atrasoFinoMs  { 0.0f };
    AlvoCanalFase alvoFase      { AlvoCanalFase::Ambos };
    float         freqCorteHpHz { 80.0f };
    float         freqCaixaHz   { 240.0f };
    float         ganhoCaixaDb  { 0.0f };
    float         suavizarRisha { 0.0f };
    float         ganhoSaidaDb  { 0.0f };
};

class AlinhadorFase
{
public:
    static constexpr float atrasoMaximoMs { 25.0f };

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        taxaAmostragem = spec.sampleRate;
        const int maxAmostras = static_cast<int> (std::ceil (taxaAmostragem * (atrasoMaximoMs * 0.001))) + 4;

        linhaAtraso.setMaximumDelayInSamples (maxAmostras);
        linhaAtraso.prepare (spec);

        atrasoSuavizado.reset (taxaAmostragem, 0.03);
        polaridadeSuavizada.reset (taxaAmostragem, 0.015);

        atrasoSuavizado.setCurrentAndTargetValue (0.0f);
        polaridadeSuavizada.setCurrentAndTargetValue (1.0f);
    }

    void reset()
    {
        linhaAtraso.reset();
        atrasoSuavizado.setCurrentAndTargetValue (atrasoSuavizado.getTargetValue());
        polaridadeSuavizada.setCurrentAndTargetValue (polaridadeSuavizada.getTargetValue());
    }

    void definirAlvos (float atrasoMs, bool inverter, AlvoCanalFase alvo)
    {
        const float limitadoMs = juce::jlimit (0.0f, atrasoMaximoMs, atrasoMs);
        const float alvoAmostras = static_cast<float> (limitadoMs * 0.001 * taxaAmostragem);
        atrasoSuavizado.setTargetValue (alvoAmostras);
        polaridadeSuavizada.setTargetValue (inverter ? -1.0f : 1.0f);
        alvoCanal = alvo;
    }

    bool canalEstaAtivo (int canal) const noexcept
    {
        switch (alvoCanal)
        {
            case AlvoCanalFase::Ambos:  return true;
            case AlvoCanalFase::Canal1: return canal == 0;
            case AlvoCanalFase::Canal2: return canal == 1;
        }
        return true;
    }

    void processarAmostra (int canal, float& amostra) noexcept
    {
        const float atrasoAtual = atrasoSuavizado.getNextValue();
        const float polaridade  = polaridadeSuavizada.getNextValue();

        linhaAtraso.pushSample (canal, amostra);

        if (canalEstaAtivo (canal))
        {
            const float atrasada = linhaAtraso.popSample (canal, atrasoAtual, true);
            amostra = atrasada * polaridade;
        }
        else
        {
            // Mantem a linha de atraso alimentada com atraso zero no canal nao selecionado
            (void) linhaAtraso.popSample (canal, 0.0f, false);
        }
    }

private:
    double taxaAmostragem { 48000.0 };
    AlvoCanalFase alvoCanal { AlvoCanalFase::Ambos };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> linhaAtraso;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> atrasoSuavizado;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> polaridadeSuavizada;
};

class EscultorCaixa
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        taxaAmostragem = spec.sampleRate;
        const size_t numCanais = static_cast<size_t> (std::max<juce::uint32> (1, spec.numChannels));

        filtrosHp.resize (numCanais);
        filtrosCaixa.resize (numCanais);

        hpSuavizado.reset (taxaAmostragem, 0.025);
        freqCaixaSuavizada.reset (taxaAmostragem, 0.025);
        ganhoCaixaSuavizado.reset (taxaAmostragem, 0.025);

        hpSuavizado.setCurrentAndTargetValue (80.0f);
        freqCaixaSuavizada.setCurrentAndTargetValue (240.0f);
        ganhoCaixaSuavizado.setCurrentAndTargetValue (0.0f);

        atualizarCoeficientes (80.0f, 240.0f, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& f : filtrosHp) f.reset();
        for (auto& f : filtrosCaixa) f.reset();
    }

    void definirAlvos (float hpHz, float caixaHz, float ganhoDb)
    {
        hpSuavizado.setTargetValue (juce::jlimit (20.0f, 500.0f, hpHz));
        freqCaixaSuavizada.setTargetValue (juce::jlimit (80.0f, 1200.0f, caixaHz));
        ganhoCaixaSuavizado.setTargetValue (juce::jlimit (-24.0f, 18.0f, ganhoDb));
    }

    // Recalcula os coeficientes IIR uma vez por bloco, avancando os suavizadores pelo tamanho do bloco
    void atualizarPorBloco (int numAmostras) noexcept
    {
        const int passos = std::max (1, numAmostras);

        float hp    = hpSuavizado.getCurrentValue();
        float freq  = freqCaixaSuavizada.getCurrentValue();
        float ganho = ganhoCaixaSuavizado.getCurrentValue();

        if (hpSuavizado.isSmoothing())
            hp = hpSuavizado.skip (passos);

        if (freqCaixaSuavizada.isSmoothing())
            freq = freqCaixaSuavizada.skip (passos);

        if (ganhoCaixaSuavizado.isSmoothing())
            ganho = ganhoCaixaSuavizado.skip (passos);

        atualizarCoeficientes (hp, freq, ganho);
    }

    float processarAmostraCanal (size_t canal, float amostra) noexcept
    {
        float y = filtrosHp[canal].processSample (amostra);
        y = filtrosCaixa[canal].processSample (y);
        return y;
    }

private:
    void atualizarCoeficientes (float hpHz, float caixaHz, float ganhoDb) noexcept
    {
        auto coeffsHp = juce::dsp::IIR::Coefficients<float>::makeHighPass (taxaAmostragem, hpHz, 0.707f);
        const float q = 2.2f;
        auto coeffsCaixa = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            taxaAmostragem, caixaHz, q, juce::Decibels::decibelsToGain (ganhoDb));

        for (auto& f : filtrosHp) *f.coefficients = *coeffsHp;
        for (auto& f : filtrosCaixa) *f.coefficients = *coeffsCaixa;
    }

    double taxaAmostragem { 48000.0 };
    std::vector<juce::dsp::IIR::Filter<float>> filtrosHp;
    std::vector<juce::dsp::IIR::Filter<float>> filtrosCaixa;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> hpSuavizado;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freqCaixaSuavizada;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ganhoCaixaSuavizado;
};

class ModeladorRisha
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        taxaAmostragem = spec.sampleRate;
        const size_t numCanais = static_cast<size_t> (std::max<juce::uint32> (1, spec.numChannels));

        envRapido.assign (numCanais, 0.0f);
        envLento.assign (numCanais, 0.0f);

        ataqueRapidoCoeff  = coeffDeMs (0.5f);
        solturaRapidaCoeff = coeffDeMs (8.0f);
        ataqueLentoCoeff   = coeffDeMs (6.0f);
        solturaLentaCoeff  = coeffDeMs (35.0f);

        suavizarSuavizado.reset (taxaAmostragem, 0.02);
        suavizarSuavizado.setCurrentAndTargetValue (0.0f);
    }

    void reset()
    {
        std::fill (envRapido.begin(), envRapido.end(), 0.0f);
        std::fill (envLento.begin(), envLento.end(), 0.0f);
        suavizarSuavizado.setCurrentAndTargetValue (suavizarSuavizado.getTargetValue());
    }

    void definirAlvoPorcentagem (float porcentagem)
    {
        suavizarSuavizado.setTargetValue (juce::jlimit (0.0f, 1.0f, porcentagem * 0.01f));
    }

    float processarAmostraCanal (size_t canal, float amostra) noexcept
    {
        const float qtdSuavizar = suavizarSuavizado.getNextValue();
        const float absAmostra  = std::abs (amostra);

        const float cRapido = (absAmostra > envRapido[canal]) ? ataqueRapidoCoeff : solturaRapidaCoeff;
        envRapido[canal] = absAmostra + cRapido * (envRapido[canal] - absAmostra);

        const float cLento = (absAmostra > envLento[canal]) ? ataqueLentoCoeff : solturaLentaCoeff;
        envLento[canal] = absAmostra + cLento * (envLento[canal] - absAmostra);

        if (qtdSuavizar <= 1.0e-5f || envRapido[canal] <= 1.0e-6f)
            return amostra;

        const float razao = envRapido[canal] / std::max (envLento[canal], 1.0e-5f);
        const float excesso = std::max (0.0f, razao - 1.15f);

        const float reducaoMaxima = 0.65f * qtdSuavizar;
        const float reducaoGanho  = juce::jlimit (0.0f, reducaoMaxima, excesso * 0.25f * qtdSuavizar);
        const float ganho = 1.0f - reducaoGanho;

        return amostra * ganho;
    }

private:
    float coeffDeMs (float ms) const noexcept
    {
        const double seg = std::max (0.0001, static_cast<double> (ms) * 0.001);
        return static_cast<float> (std::exp (-1.0 / (taxaAmostragem * seg)));
    }

    double taxaAmostragem { 48000.0 };
    float ataqueRapidoCoeff  { 0.0f };
    float solturaRapidaCoeff { 0.0f };
    float ataqueLentoCoeff   { 0.0f };
    float solturaLentaCoeff  { 0.0f };

    std::vector<float> envRapido;
    std::vector<float> envLento;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> suavizarSuavizado;
};

class CadeiaDspOud
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        alinhadorFase.prepare (spec);
        escultorCaixa.prepare (spec);
        modeladorRisha.prepare (spec);

        ganhoSaidaSuavizado.reset (spec.sampleRate, 0.02);
        ganhoSaidaSuavizado.setCurrentAndTargetValue (1.0f);
    }

    void reset()
    {
        alinhadorFase.reset();
        escultorCaixa.reset();
        modeladorRisha.reset();
        ganhoSaidaSuavizado.setCurrentAndTargetValue (ganhoSaidaSuavizado.getTargetValue());
    }

    void atualizarParametros (const ParametrosDspOud& p)
    {
        alinhadorFase.definirAlvos (p.atrasoFinoMs, p.inverterFase, p.alvoFase);
        escultorCaixa.definirAlvos (p.freqCorteHpHz, p.freqCaixaHz, p.ganhoCaixaDb);
        modeladorRisha.definirAlvoPorcentagem (p.suavizarRisha);
        ganhoSaidaSuavizado.setTargetValue (juce::Decibels::decibelsToGain (p.ganhoSaidaDb));
    }

    void process (juce::AudioBuffer<float>& buffer) noexcept
    {
        juce::ScopedNoDenormals noDenormals;

        const int numCanais   = buffer.getNumChannels();
        const int numAmostras = buffer.getNumSamples();

        if (numCanais == 0 || numAmostras == 0)
            return;

        // Atualiza os filtros IIR uma unica vez por bloco
        escultorCaixa.atualizarPorBloco (numAmostras);

        for (int n = 0; n < numAmostras; ++n)
        {
            const float ganhoSaida = ganhoSaidaSuavizado.getNextValue();

            for (int ch = 0; ch < numCanais; ++ch)
            {
                const size_t idxCh = static_cast<size_t> (ch);
                float amostra = buffer.getSample (ch, n);

                alinhadorFase.processarAmostra (ch, amostra);
                amostra = escultorCaixa.processarAmostraCanal (idxCh, amostra);
                amostra = modeladorRisha.processarAmostraCanal (idxCh, amostra);
                amostra *= ganhoSaida;

                buffer.setSample (ch, n, amostra);
            }
        }
    }

private:
    AlinhadorFase  alinhadorFase;
    EscultorCaixa  escultorCaixa;
    ModeladorRisha modeladorRisha;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> ganhoSaidaSuavizado;
};
