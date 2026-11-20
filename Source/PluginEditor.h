#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "OudLookAndFeel.h"

class OudBodyPhaseAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit OudBodyPhaseAudioProcessorEditor (OudBodyPhaseAudioProcessor&);
    ~OudBodyPhaseAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    OudBodyPhaseAudioProcessor& audioProcessor;
    OudLookAndFeel oudLookAndFeel;

    juce::TextButton botaoTema { "Nahar" };

    // Controles de Fase e Alinhamento
    juce::ToggleButton botaoFase { "Inverter Fase (180°)" };
    juce::Label rotuloAlvoFase;
    juce::ComboBox caixaAlvoFase;
    juce::Label rotuloAtraso;
    juce::Slider sliderAtraso;

    // Controles de Corpo e Caixa
    juce::Label rotuloHp;
    juce::Slider sliderHp;
    juce::Label rotuloCaixaFreq;
    juce::Slider sliderCaixaFreq;
    juce::Label rotuloCaixaGanho;
    juce::Slider sliderCaixaGanho;

    // Controle de Risha
    juce::Label rotuloRisha;
    juce::Slider sliderRisha;

    // Controle de Saida
    juce::Label rotuloSaida;
    juce::Slider sliderSaida;

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<ButtonAttachment>   attachFase;
    std::unique_ptr<ComboBoxAttachment> attachAlvoFase;
    std::unique_ptr<SliderAttachment>   attachAtraso;
    std::unique_ptr<SliderAttachment>   attachHp;
    std::unique_ptr<SliderAttachment>   attachCaixaFreq;
    std::unique_ptr<SliderAttachment>   attachCaixaGanho;
    std::unique_ptr<SliderAttachment>   attachRisha;
    std::unique_ptr<SliderAttachment>   attachSaida;

    juce::Rectangle<int> cartaoCabecalho;
    juce::Rectangle<int> painelFase;
    juce::Rectangle<int> painelCaixa;
    juce::Rectangle<int> painelRisha;
    juce::Rectangle<int> painelSaida;

    void alternarTema();
    void aplicarCoresComponentes();
    void configurarRotulo (juce::Label& rotulo, const juce::String& texto);
    void configurarKnob (juce::Slider& slider);
    void desenharCartaoSecao (juce::Graphics& g,
                               juce::Rectangle<int> area,
                               const juce::String& titulo,
                               juce::Colour corTitulo);
    void posicionarKnob (juce::Rectangle<int> coluna, juce::Label& rotulo, juce::Slider& knob);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OudBodyPhaseAudioProcessorEditor)
};
