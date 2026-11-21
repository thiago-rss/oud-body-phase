#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

OudBodyPhaseAudioProcessor::OudBodyPhaseAudioProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETROS_OUD", criarLayoutParametrosOud())
{
}

bool OudBodyPhaseAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != mainOut)
        return false;

    return mainIn == juce::AudioChannelSet::mono()
        || mainIn == juce::AudioChannelSet::stereo();
}

void OudBodyPhaseAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const int canaisAtivos = std::max (1, getMainBusNumInputChannels());
    numCanaisAtivos.store (canaisAtivos, std::memory_order_relaxed);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (std::max (1, samplesPerBlock));
    spec.numChannels = static_cast<juce::uint32> (std::max (2, canaisAtivos));

    cadeiaDsp.prepare (spec);
    cadeiaDsp.reset();
    setLatencySamples (CadeiaDspOud::getLatenciaAmostras());

    medidorReducaoRisha.reset();
    medidorPicoSaida.reset();
    medidorCorrelacaoFase.reset();
}

void OudBodyPhaseAudioProcessor::releaseResources()
{
    cadeiaDsp.reset();
    medidorReducaoRisha.reset();
    medidorPicoSaida.reset();
    medidorCorrelacaoFase.reset();
}

void OudBodyPhaseAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalCanaisEntrada = getTotalNumInputChannels();
    const auto totalCanaisSaida   = getTotalNumOutputChannels();

    numCanaisAtivos.store (std::max (1, buffer.getNumChannels()), std::memory_order_relaxed);

    for (auto i = totalCanaisEntrada; i < totalCanaisSaida; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    ParametrosDspOud p;
    p.inverterFase  = apvts.getRawParameterValue (OudParamIDs::inverterFase)->load() > 0.5f;
    p.atrasoFinoMs  = apvts.getRawParameterValue (OudParamIDs::atrasoFinoMs)->load();

    const int escolhaAlvo = static_cast<int> (
        std::lround (apvts.getRawParameterValue (OudParamIDs::alvoFase)->load()));
    p.alvoFase      = static_cast<AlvoCanalFase> (juce::jlimit (0, 2, escolhaAlvo));

    p.freqCorteHpHz = apvts.getRawParameterValue (OudParamIDs::freqCorteHpHz)->load();
    p.freqCaixaHz   = apvts.getRawParameterValue (OudParamIDs::freqCaixaHz)->load();
    p.ganhoCaixaDb  = apvts.getRawParameterValue (OudParamIDs::ganhoCaixaDb)->load();
    p.suavizarRisha = apvts.getRawParameterValue (OudParamIDs::suavizarRisha)->load();
    p.ganhoSaidaDb  = apvts.getRawParameterValue (OudParamIDs::ganhoSaidaDb)->load();

    cadeiaDsp.atualizarParametros (p);

    const auto leitura = cadeiaDsp.process (buffer);
    medidorReducaoRisha.registrarPico (leitura.reducaoRishaDb);
    medidorPicoSaida.registrarPico (leitura.picoSaidaLinear);
    medidorCorrelacaoFase.registrar (leitura.correlacao);
}

juce::AudioProcessorEditor* OudBodyPhaseAudioProcessor::createEditor()
{
    return new OudBodyPhaseAudioProcessorEditor (*this);
}

void OudBodyPhaseAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void OudBodyPhaseAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OudBodyPhaseAudioProcessor();
}
