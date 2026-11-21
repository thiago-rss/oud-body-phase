#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <algorithm>
#include <atomic>
#include "OudParameters.h"
#include "OudDspChain.h"

class OudBodyPhaseAudioProcessor : public juce::AudioProcessor
{
public:
    OudBodyPhaseAudioProcessor();
    ~OudBodyPhaseAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    float consumirReducaoRishaDb() noexcept                 { return medidorReducaoRisha.consumirPico(); }
    float consumirPicoSaidaLinear() noexcept                { return medidorPicoSaida.consumirPico(); }
    LeituraCorrelacaoOud lerCorrelacaoFase() const noexcept { return medidorCorrelacaoFase.ler(); }

    int getNumCanaisAtivos() const noexcept
    {
        const int atomico = numCanaisAtivos.load (std::memory_order_relaxed);
        return (atomico > 0) ? atomico : std::max (1, getMainBusNumInputChannels());
    }

private:
    CadeiaDspOud cadeiaDsp;
    MedidorAtomico medidorReducaoRisha;
    MedidorAtomico medidorPicoSaida;
    MedidorCorrelacaoAtomico medidorCorrelacaoFase;
    std::atomic<int> numCanaisAtivos { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OudBodyPhaseAudioProcessor)
};
