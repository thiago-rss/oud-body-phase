#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace OudParamIDs
{
    inline constexpr const char* inverterFase  { "inverter_fase" };
    inline constexpr const char* atrasoFinoMs  { "atraso_fino_ms" };
    inline constexpr const char* alvoFase      { "alvo_fase" };
    inline constexpr const char* freqCorteHpHz { "freq_corte_hp_hz" };
    inline constexpr const char* freqCaixaHz   { "freq_caixa_hz" };
    inline constexpr const char* ganhoCaixaDb  { "ganho_caixa_db" };
    inline constexpr const char* suavizarRisha { "suavizar_risha" };
    inline constexpr const char* ganhoSaidaDb  { "ganho_saida_db" };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout criarLayoutParametrosOud()
{
    using AP = juce::AudioParameterFloat;
    using AB = juce::AudioParameterBool;
    using AC = juce::AudioParameterChoice;
    using NR = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<AB> (
        juce::ParameterID { OudParamIDs::inverterFase, 1 },
        "Inverter Fase",
        false));

    params.push_back (std::make_unique<AP> (
        juce::ParameterID { OudParamIDs::atrasoFinoMs, 1 },
        "Atraso Fino",
        NR { 0.0f, 15.0f, 0.01f, 0.65f },
        0.0f,
        juce::AudioParameterFloatAttributes {}.withLabel ("ms")));

    params.push_back (std::make_unique<AC> (
        juce::ParameterID { OudParamIDs::alvoFase, 1 },
        "Alvo Fase / Atraso",
        juce::StringArray { "Ambos", "Canal 1", "Canal 2" },
        0));

    params.push_back (std::make_unique<AP> (
        juce::ParameterID { OudParamIDs::freqCorteHpHz, 1 },
        "Corte HPF",
        NR { 30.0f, 250.0f, 0.1f, 0.45f },
        80.0f,
        juce::AudioParameterFloatAttributes {}.withLabel ("Hz")));

    params.push_back (std::make_unique<AP> (
        juce::ParameterID { OudParamIDs::freqCaixaHz, 1 },
        "Frequencia da Caixa",
        NR { 140.0f, 520.0f, 0.1f, 0.5f },
        240.0f,
        juce::AudioParameterFloatAttributes {}.withLabel ("Hz")));

    params.push_back (std::make_unique<AP> (
        juce::ParameterID { OudParamIDs::ganhoCaixaDb, 1 },
        "Corte / Realce Caixa",
        NR { -12.0f, 6.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes {}.withLabel ("dB")));

    params.push_back (std::make_unique<AP> (
        juce::ParameterID { OudParamIDs::suavizarRisha, 1 },
        "Suavizar Risha",
        NR { 0.0f, 100.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes {}.withLabel ("%")));

    params.push_back (std::make_unique<AP> (
        juce::ParameterID { OudParamIDs::ganhoSaidaDb, 1 },
        "Ganho de Saida",
        NR { -18.0f, 12.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes {}.withLabel ("dB")));

    return { params.begin(), params.end() };
}
