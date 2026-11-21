#pragma once

#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <atomic>
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
    bool inverterFase      = false;
    float atrasoFinoMs     = 0.0f;
    AlvoCanalFase alvoFase = AlvoCanalFase::Ambos;
    float freqCorteHpHz    = 80.0f;
    float freqCaixaHz      = 240.0f;
    float ganhoCaixaDb     = 0.0f;
    float suavizarRisha    = 0.0f;
    float ganhoSaidaDb     = 0.0f;
};

struct LeituraMedidoresOud
{
    float reducaoRishaDb  = 0.0f; // Valor positivo em dB (0.0 a ~12.0 dB)
    float picoSaidaLinear = 0.0f; // Amplitude linear (0.0 a >= 1.0)
};

// Ponte lock-free (audio thread -> UI thread) que preserva picos curtos entre quadros do Timer
struct MedidorAtomico
{
    void registrarPico (float novoValor) noexcept
    {
        if (! std::isfinite (novoValor) || novoValor <= 0.0f)
            return;

        float atual = pico.load (std::memory_order_relaxed);
        while (novoValor > atual
               && ! pico.compare_exchange_weak (atual, novoValor, std::memory_order_relaxed))
        {
        }
    }

    float consumirPico() noexcept
    {
        return pico.exchange (0.0f, std::memory_order_relaxed);
    }

    void reset() noexcept
    {
        pico.store (0.0f, std::memory_order_relaxed);
    }

private:
    std::atomic<float> pico { 0.0f };
};

class AlinhadorFase
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        taxaAmostragem = std::max (1.0, spec.sampleRate);
        maxAmostrasAtraso = static_cast<int> (std::ceil (0.025 * taxaAmostragem)) + 4;

        linhaAtraso = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> (maxAmostrasAtraso);
        linhaAtraso.prepare (spec);

        atrasoAmostrasSuave.reset (taxaAmostragem, 0.030);
        polaridadeSuave.reset (taxaAmostragem, 0.015);
        reset();
    }

    void reset()
    {
        linhaAtraso.reset();
        atrasoAmostrasSuave.setCurrentAndTargetValue (0.0f);
        polaridadeSuave.setCurrentAndTargetValue (1.0f);
    }

    void atualizar (bool inverter, float atrasoMs, AlvoCanalFase alvo)
    {
        alvoAtual = alvo;
        const float amostras = juce::jlimit (
            0.0f,
            static_cast<float> (maxAmostrasAtraso - 2),
            (atrasoMs * 0.001f) * static_cast<float> (taxaAmostragem));

        atrasoAmostrasSuave.setTargetValue (amostras);
        polaridadeSuave.setTargetValue (inverter ? -1.0f : 1.0f);
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCanais   = buffer.getNumChannels();
        const int numAmostras = buffer.getNumSamples();

        for (int n = 0; n < numAmostras; ++n)
        {
            const float atrasoAtual = atrasoAmostrasSuave.getNextValue();
            const float polAtual    = polaridadeSuave.getNextValue();

            for (int ch = 0; ch < numCanais; ++ch)
            {
                auto* dados = buffer.getWritePointer (ch);
                const float entrada = dados[n];
                linhaAtraso.pushSample (ch, entrada);

                const bool aplicarNoCanal =
                    (alvoAtual == AlvoCanalFase::Ambos)
                    || (alvoAtual == AlvoCanalFase::Canal1 && ch == 0)
                    || (alvoAtual == AlvoCanalFase::Canal2 && ch == 1);

                if (aplicarNoCanal)
                {
                    const float atrasada = linhaAtraso.popSample (ch, atrasoAtual);
                    dados[n] = atrasada * polAtual;
                }
                else
                {
                    // Mantem o buffer da linha coerente caso o alvo mude durante a reproducao
                    (void) linhaAtraso.popSample (ch, 0.0f);
                }
            }
        }
    }

private:
    double taxaAmostragem = 48000.0;
    int maxAmostrasAtraso = 2048;
    AlvoCanalFase alvoAtual = AlvoCanalFase::Ambos;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> linhaAtraso { 2048 };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> atrasoAmostrasSuave;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> polaridadeSuave;
};

class EscultorCaixa
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        taxaAmostragem = std::max (1.0, spec.sampleRate);
        const size_t canais = std::max<size_t> (1, spec.numChannels);

        filtrosHp.assign (canais, Biquad {});
        filtrosCaixa.assign (canais, Biquad {});

        freqHpSuave.reset (taxaAmostragem, 0.025);
        freqCaixaSuave.reset (taxaAmostragem, 0.025);
        ganhoCaixaSuave.reset (taxaAmostragem, 0.025);

        freqHpSuave.setCurrentAndTargetValue (80.0f);
        freqCaixaSuave.setCurrentAndTargetValue (240.0f);
        ganhoCaixaSuave.setCurrentAndTargetValue (0.0f);

        atualizarCoeficientes (80.0f, 240.0f, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& f : filtrosHp)    f.reset();
        for (auto& f : filtrosCaixa) f.reset();
    }

    void atualizar (float freqHpHz, float freqCaixaHz, float ganhoCaixaDb)
    {
        freqHpSuave.setTargetValue (juce::jlimit (20.0f, 400.0f, freqHpHz));
        freqCaixaSuave.setTargetValue (juce::jlimit (100.0f, 800.0f, freqCaixaHz));
        ganhoCaixaSuave.setTargetValue (juce::jlimit (-18.0f, 12.0f, ganhoCaixaDb));
    }

    void process (juce::AudioBuffer<float>& buffer)
    {
        const int numCanais   = buffer.getNumChannels();
        const int numAmostras = buffer.getNumSamples();

        if (numAmostras <= 0)
            return;

        // Atualiza os coeficientes IIR uma unica vez por bloco
        const float hpBloco    = freqHpSuave.skip (numAmostras);
        const float caixaBloco = freqCaixaSuave.skip (numAmostras);
        const float ganhoBloco = ganhoCaixaSuave.skip (numAmostras);
        atualizarCoeficientes (hpBloco, caixaBloco, ganhoBloco);

        for (int ch = 0; ch < numCanais && ch < static_cast<int> (filtrosHp.size()); ++ch)
        {
            auto* dados = buffer.getWritePointer (ch);
            auto& hp    = filtrosHp[static_cast<size_t> (ch)];
            auto& caixa = filtrosCaixa[static_cast<size_t> (ch)];

            for (int n = 0; n < numAmostras; ++n)
            {
                const float aposHp = hp.processar (dados[n]);
                dados[n] = caixa.processar (aposHp);
            }
        }
    }

private:
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0;
        double a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        void reset() noexcept { z1 = 0.0; z2 = 0.0; }

        void definirHighPass (double sr, double freq, double q) noexcept
        {
            const double w0    = juce::MathConstants<double>::twoPi * (freq / sr);
            const double cosW0 = std::cos (w0);
            const double alpha = std::sin (w0) / (2.0 * q);
            const double invA0 = 1.0 / (1.0 + alpha);

            b0 = ((1.0 + cosW0) * 0.5) * invA0;
            b1 = (-(1.0 + cosW0)) * invA0;
            b2 = ((1.0 + cosW0) * 0.5) * invA0;
            a1 = (-2.0 * cosW0) * invA0;
            a2 = (1.0 - alpha) * invA0;
        }

        void definirPeak (double sr, double freq, double q, double ganhoDb) noexcept
        {
            const double a     = std::pow (10.0, ganhoDb / 40.0);
            const double w0    = juce::MathConstants<double>::twoPi * (freq / sr);
            const double cosW0 = std::cos (w0);
            const double alpha = std::sin (w0) / (2.0 * q);
            const double invA0 = 1.0 / (1.0 + alpha / a);

            b0 = (1.0 + alpha * a) * invA0;
            b1 = (-2.0 * cosW0) * invA0;
            b2 = (1.0 - alpha * a) * invA0;
            a1 = (-2.0 * cosW0) * invA0;
            a2 = (1.0 - alpha / a) * invA0;
        }

        float processar (float x) noexcept
        {
            const double xd = static_cast<double> (x);
            const double y  = b0 * xd + z1;
            z1 = b1 * xd - a1 * y + z2;
            z2 = b2 * xd - a2 * y;
            return static_cast<float> (y);
        }
    };

    void atualizarCoeficientes (float freqHp, float freqCaixa, float ganhoDb) noexcept
    {
        const double maxFreq = taxaAmostragem * 0.45;
        const double hp      = juce::jlimit (20.0, maxFreq, static_cast<double> (freqHp));
        const double cx      = juce::jlimit (80.0, maxFreq, static_cast<double> (freqCaixa));

        for (auto& f : filtrosHp)    f.definirHighPass (taxaAmostragem, hp, 0.70710678);
        for (auto& f : filtrosCaixa) f.definirPeak (taxaAmostragem, cx, 2.2, static_cast<double> (ganhoDb));
    }

    double taxaAmostragem = 48000.0;
    std::vector<Biquad> filtrosHp;
    std::vector<Biquad> filtrosCaixa;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freqHpSuave;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> freqCaixaSuave;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ganhoCaixaSuave;
};

class ModeladorRisha
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        taxaAmostragem = std::max (1.0, spec.sampleRate);
        const size_t canais = std::max<size_t> (1, spec.numChannels);

        envRapido.assign (canais, 0.0f);
        envLento.assign (canais, 0.0f);
        ganhoSuaveCanal.assign (canais, 1.0f);

        coeffAtaqueRapido  = calcularCoeficiente (0.0005); // 0.5 ms
        coeffSolturaRapida = calcularCoeficiente (0.0080); // 8 ms
        coeffAtaqueLento   = calcularCoeficiente (0.0060); // 6 ms
        coeffSolturaLenta  = calcularCoeficiente (0.0350); // 35 ms
        coeffGanho         = calcularCoeficiente (0.0020); // 2 ms

        intensidadeSuave.reset (taxaAmostragem, 0.020);
        intensidadeSuave.setCurrentAndTargetValue (0.0f);
        reset();
    }

    void reset()
    {
        std::fill (envRapido.begin(), envRapido.end(), 0.0f);
        std::fill (envLento.begin(), envLento.end(), 0.0f);
        std::fill (ganhoSuaveCanal.begin(), ganhoSuaveCanal.end(), 1.0f);
    }

    void atualizar (float suavizarPorcento)
    {
        intensidadeSuave.setTargetValue (juce::jlimit (0.0f, 1.0f, suavizarPorcento * 0.01f));
    }

    // Retorna a maior reducao de ganho aplicada no bloco (em dB positivos, 0.0 a ~12.0 dB)
    float process (juce::AudioBuffer<float>& buffer)
    {
        const int numCanais   = buffer.getNumChannels();
        const int numAmostras = buffer.getNumSamples();
        float menorGanhoBloco = 1.0f;

        for (int n = 0; n < numAmostras; ++n)
        {
            const float intensidade = intensidadeSuave.getNextValue();

            for (int ch = 0; ch < numCanais && ch < static_cast<int> (envRapido.size()); ++ch)
            {
                auto* dados = buffer.getWritePointer (ch);
                const float x    = dados[n];
                const float absX = std::abs (x);

                const auto idx = static_cast<size_t> (ch);
                const float cRap = (absX > envRapido[idx]) ? coeffAtaqueRapido : coeffSolturaRapida;
                const float cLen = (absX > envLento[idx])  ? coeffAtaqueLento  : coeffSolturaLenta;

                envRapido[idx] = cRap * envRapido[idx] + (1.0f - cRap) * absX;
                envLento[idx]  = cLen * envLento[idx]  + (1.0f - cLen) * absX;

                float ganhoAlvo = 1.0f;
                if (intensidade > 0.0001f && envRapido[idx] > 1.0e-5f)
                {
                    const float razao = (envRapido[idx] + 1.0e-5f) / (envLento[idx] + 1.0e-5f);
                    if (razao > 1.15f)
                    {
                        const float excesso   = std::min (razao - 1.15f, 4.0f);
                        const float atenuacao = (excesso / (1.0f + excesso)) * intensidade * 0.75f;
                        ganhoAlvo = juce::jlimit (0.25f, 1.0f, 1.0f - atenuacao);
                    }
                }

                ganhoSuaveCanal[idx] = coeffGanho * ganhoSuaveCanal[idx] + (1.0f - coeffGanho) * ganhoAlvo;
                dados[n] = x * ganhoSuaveCanal[idx];

                if (ganhoSuaveCanal[idx] < menorGanhoBloco)
                    menorGanhoBloco = ganhoSuaveCanal[idx];
            }
        }

        const float reducaoDb = -juce::Decibels::gainToDecibels (menorGanhoBloco, -24.0f);
        return std::max (0.0f, reducaoDb);
    }

private:
    float calcularCoeficiente (double tempoSeg) const noexcept
    {
        return static_cast<float> (std::exp (-1.0 / (std::max (0.0001, tempoSeg) * taxaAmostragem)));
    }

    double taxaAmostragem = 48000.0;
    float coeffAtaqueRapido  = 0.0f;
    float coeffSolturaRapida = 0.0f;
    float coeffAtaqueLento   = 0.0f;
    float coeffSolturaLenta  = 0.0f;
    float coeffGanho         = 0.0f;

    std::vector<float> envRapido;
    std::vector<float> envLento;
    std::vector<float> ganhoSuaveCanal;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> intensidadeSuave;
};

class CadeiaDspOud
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        alinhadorFase.prepare (spec);
        escultorCaixa.prepare (spec);
        modeladorRisha.prepare (spec);

        ganhoSaidaSuave.reset (std::max (1.0, spec.sampleRate), 0.020);
        ganhoSaidaSuave.setCurrentAndTargetValue (1.0f);
    }

    void reset()
    {
        alinhadorFase.reset();
        escultorCaixa.reset();
        modeladorRisha.reset();
        ganhoSaidaSuave.setCurrentAndTargetValue (ganhoSaidaSuave.getTargetValue());
    }

    void atualizarParametros (const ParametrosDspOud& p)
    {
        alinhadorFase.atualizar (p.inverterFase, p.atrasoFinoMs, p.alvoFase);
        escultorCaixa.atualizar (p.freqCorteHpHz, p.freqCaixaHz, p.ganhoCaixaDb);
        modeladorRisha.atualizar (p.suavizarRisha);
        ganhoSaidaSuave.setTargetValue (juce::Decibels::decibelsToGain (p.ganhoSaidaDb));
    }

    LeituraMedidoresOud process (juce::AudioBuffer<float>& buffer)
    {
        LeituraMedidoresOud leitura;

        alinhadorFase.process (buffer);
        escultorCaixa.process (buffer);
        leitura.reducaoRishaDb = modeladorRisha.process (buffer);

        const int numCanais   = buffer.getNumChannels();
        const int numAmostras = buffer.getNumSamples();

        for (int n = 0; n < numAmostras; ++n)
        {
            const float ganho = ganhoSaidaSuave.getNextValue();
            for (int ch = 0; ch < numCanais; ++ch)
                buffer.getWritePointer (ch)[n] *= ganho;
        }

        leitura.picoSaidaLinear = buffer.getMagnitude (0, numAmostras);
        return leitura;
    }

private:
    AlinhadorFase alinhadorFase;
    EscultorCaixa escultorCaixa;
    ModeladorRisha modeladorRisha;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> ganhoSaidaSuave;
};
