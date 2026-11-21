#include "PluginEditor.h"
#include <algorithm>

OudBodyPhaseAudioProcessorEditor::OudBodyPhaseAudioProcessorEditor (OudBodyPhaseAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&oudLookAndFeel);

    botaoTema.setTooltip (juce::String::fromUTF8 ("Alternar entre tema " "\xc3\x89" "bano e tema claro Nahar"));
    botaoTema.onClick = [this] { alternarTema(); };
    addAndMakeVisible (botaoTema);

    botaoFase.setButtonText (juce::String::fromUTF8 ("Inverter Fase (180" "\xc2\xb0" ")"));
    addAndMakeVisible (botaoFase);

    configurarRotulo (rotuloAlvoFase, "Alvo");
    caixaAlvoFase.addItemList ({ "Ambos", "Canal 1", "Canal 2" }, 1);
    caixaAlvoFase.setTooltip (juce::String::fromUTF8 (
        "Escolha Canal 1 ou Canal 2 para inverter/alinhar uma fonte contra a outra (Ambos mant" "\xc3\xa9" "m a fase relativa)"));
    addAndMakeVisible (caixaAlvoFase);

    configurarRotulo (rotuloAtraso, "Atraso");
    configurarRotulo (rotuloHp, "HPF");
    configurarRotulo (rotuloCaixaFreq, "Freq Caixa");
    configurarRotulo (rotuloCaixaGanho, "Ganho Caixa");
    configurarRotulo (rotuloRisha, "Suavizar Risha");
    configurarRotulo (rotuloSaida, juce::String::fromUTF8 ("Ganho Sa" "\xc3\xad" "da"));

    configurarKnob (sliderAtraso);
    configurarKnob (sliderHp);
    configurarKnob (sliderCaixaFreq);
    configurarKnob (sliderCaixaGanho);
    configurarKnob (sliderRisha);
    configurarKnob (sliderSaida);

    addAndMakeVisible (medidorCorrelacao);
    addAndMakeVisible (medidorRisha);
    addAndMakeVisible (medidorSaida);

    attachFase       = std::make_unique<ButtonAttachment>   (audioProcessor.apvts, OudParamIDs::inverterFase,  botaoFase);
    attachAlvoFase   = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, OudParamIDs::alvoFase,      caixaAlvoFase);
    attachAtraso     = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::atrasoFinoMs,  sliderAtraso);
    attachHp         = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::freqCorteHpHz, sliderHp);
    attachCaixaFreq  = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::freqCaixaHz,   sliderCaixaFreq);
    attachCaixaGanho = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::ganhoCaixaDb,  sliderCaixaGanho);
    attachRisha      = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::suavizarRisha, sliderRisha);
    attachSaida      = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::ganhoSaidaDb,  sliderSaida);

    aplicarCoresComponentes();
    setSize (900, 336);
    atualizarModoCanais (true);
    startTimerHz (30);
}

OudBodyPhaseAudioProcessorEditor::~OudBodyPhaseAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void OudBodyPhaseAudioProcessorEditor::timerCallback()
{
    atualizarModoCanais (false);

    if (! modoMono)
        medidorCorrelacao.atualizarLeitura (audioProcessor.lerCorrelacaoFase());

    medidorRisha.atualizarLeitura (audioProcessor.consumirReducaoRishaDb());
    medidorSaida.atualizarLeitura (audioProcessor.consumirPicoSaidaLinear());
}

void OudBodyPhaseAudioProcessorEditor::atualizarModoCanais (bool forcarLayout)
{
    const bool agoraMono = (audioProcessor.getNumCanaisAtivos() < 2);
    if (! forcarLayout && agoraMono == modoMono)
        return;

    modoMono = agoraMono;
    const bool exibirControlesEstereo = ! modoMono;

    rotuloAlvoFase.setVisible (exibirControlesEstereo);
    caixaAlvoFase.setVisible (exibirControlesEstereo);
    caixaAlvoFase.setEnabled (exibirControlesEstereo);
    medidorCorrelacao.setVisible (exibirControlesEstereo);

    resized();
    repaint();
}

void OudBodyPhaseAudioProcessorEditor::alternarTema()
{
    const bool proximoClaro = ! oudLookAndFeel.estaTemaClaro();
    oudLookAndFeel.aplicarTema (proximoClaro);
    botaoTema.setButtonText (proximoClaro ? juce::String::fromUTF8 ("\xc3\x89" "bano") : "Nahar");
    aplicarCoresComponentes();
    repaint();
}

void OudBodyPhaseAudioProcessorEditor::aplicarCoresComponentes()
{
    const auto& pal = oudLookAndFeel.getPaleta();
    const bool temaClaro = oudLookAndFeel.estaTemaClaro();

    auto atualizarLabel = [&] (juce::Label& lbl, bool destaque = false)
    {
        lbl.setFont (oudLookAndFeel.getFonteInterface (11.5f, true));
        lbl.setColour (juce::Label::textColourId, destaque ? pal.textoSecundario : pal.textoSuave);
    };

    atualizarLabel (rotuloAlvoFase, true);
    atualizarLabel (rotuloAtraso);
    atualizarLabel (rotuloHp);
    atualizarLabel (rotuloCaixaFreq);
    atualizarLabel (rotuloCaixaGanho);
    atualizarLabel (rotuloRisha);
    atualizarLabel (rotuloSaida);

    auto atualizarSlider = [&] (juce::Slider& s)
    {
        s.setColour (juce::Slider::textBoxTextColourId, pal.textoPrincipal);
        s.setColour (juce::Slider::textBoxBackgroundColourId, pal.input);
        s.setColour (juce::Slider::textBoxOutlineColourId, pal.bordaSutil);
    };

    atualizarSlider (sliderAtraso);
    atualizarSlider (sliderHp);
    atualizarSlider (sliderCaixaFreq);
    atualizarSlider (sliderCaixaGanho);
    atualizarSlider (sliderRisha);
    atualizarSlider (sliderSaida);

    medidorCorrelacao.setTemaClaro (temaClaro);
    medidorRisha.setTemaClaro (temaClaro);
    medidorSaida.setTemaClaro (temaClaro);
}

void OudBodyPhaseAudioProcessorEditor::configurarRotulo (juce::Label& rotulo, const juce::String& texto)
{
    rotulo.setText (texto, juce::dontSendNotification);
    rotulo.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (rotulo);
}

void OudBodyPhaseAudioProcessorEditor::configurarKnob (juce::Slider& slider)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    addAndMakeVisible (slider);
}

void OudBodyPhaseAudioProcessorEditor::desenharCartaoSecao (juce::Graphics& g,
                                                            juce::Rectangle<int> area,
                                                            const juce::String& titulo,
                                                            juce::Colour corTitulo)
{
    if (area.isEmpty())
        return;

    const auto& pal = oudLookAndFeel.getPaleta();
    const auto rectF = area.toFloat();

    g.setColour (pal.superficie);
    g.fillRoundedRectangle (rectF, 13.0f);

    g.setColour (pal.bordaSutil);
    g.drawRoundedRectangle (rectF, 13.0f, 1.0f);

    g.setColour (corTitulo);
    g.setFont (oudLookAndFeel.getFonteInterface (10.5f, true));
    g.drawText (titulo.toUpperCase(),
                area.reduced (12, 8).removeFromTop (15),
                juce::Justification::centredLeft, false);
}

void OudBodyPhaseAudioProcessorEditor::posicionarKnob (juce::Rectangle<int> coluna,
                                                       juce::Label& rotulo,
                                                       juce::Slider& knob)
{
    auto conteudo = coluna.reduced (4, 2);
    rotulo.setBounds (conteudo.removeFromTop (18));
    knob.setBounds (conteudo);
}

void OudBodyPhaseAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto& pal = oudLookAndFeel.getPaleta();

    juce::ColourGradient gradienteFundo (
        pal.fundoDeep.brighter (0.04f), 0.0f, 0.0f,
        pal.fundoDeep.darker (0.12f), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (gradienteFundo);
    g.fillAll();

    if (! cartaoCabecalho.isEmpty())
    {
        const auto cabF = cartaoCabecalho.toFloat();
        g.setColour (pal.superficie);
        g.fillRoundedRectangle (cabF, 14.0f);
        g.setColour (pal.bordaSutil);
        g.drawRoundedRectangle (cabF, 14.0f, 1.0f);

        auto interno = cartaoCabecalho.reduced (14, 6);
        interno.removeFromRight (88);

        g.setColour (pal.textoPrincipal);
        g.setFont (oudLookAndFeel.getFonteTitulo (22.0f));
        g.drawText ("Oud Body Phase", interno.removeFromLeft (195), juce::Justification::centredLeft);

        g.setColour (pal.textoSuave);
        g.setFont (oudLookAndFeel.getFonteInterface (11.5f, false));
        g.drawText ("Fase, Escultura de Caixa & Risha", interno, juce::Justification::centredLeft);
    }

    desenharCartaoSecao (g, painelFase,  "Fase & Alinhamento", pal.latao);
    desenharCartaoSecao (g, painelCaixa, "Corpo & Caixa",      pal.latao);
    desenharCartaoSecao (g, painelRisha, "Risha",              pal.ameixa);
    desenharCartaoSecao (g, painelSaida, juce::String::fromUTF8 ("Sa" "\xc3\xad" "da"), pal.latao);
}

void OudBodyPhaseAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (14, 12);

    cartaoCabecalho = area.removeFromTop (46);
    {
        auto internoCab = cartaoCabecalho.reduced (14, 6);
        auto direitaCab = internoCab.removeFromRight (88);
        botaoTema.setBounds (direitaCab.withSizeKeepingCentre (76, 28));
    }

    area.removeFromTop (10);

    const int gap = 10;
    const int larguraUtil = area.getWidth() - gap * 3;

    const int wFase  = static_cast<int> (larguraUtil * 0.33f);
    const int wCaixa = static_cast<int> (larguraUtil * 0.33f);
    const int wRisha = static_cast<int> (larguraUtil * 0.17f);
    const int wSaida = std::max (120, larguraUtil - wFase - wCaixa - wRisha);

    painelFase  = area.removeFromLeft (wFase);
    area.removeFromLeft (gap);
    painelCaixa = area.removeFromLeft (wCaixa);
    area.removeFromLeft (gap);
    painelRisha = area.removeFromLeft (wRisha);
    area.removeFromLeft (gap);
    painelSaida = area;

    // 1. Fase & Alinhamento (adaptativo para Mono vs Estereo)
    {
        auto inner = painelFase.reduced (10, 8);
        inner.removeFromTop (18);

        auto linhaTopo = inner.removeFromTop (28);
        if (modoMono)
        {
            botaoFase.setBounds (linhaTopo);
        }
        else
        {
            botaoFase.setBounds (linhaTopo.removeFromLeft (144));
            linhaTopo.removeFromLeft (4);
            rotuloAlvoFase.setBounds (linhaTopo.removeFromLeft (28));
            caixaAlvoFase.setBounds (linhaTopo);

            auto areaCorrelacao = inner.removeFromBottom (26);
            inner.removeFromBottom (4);
            medidorCorrelacao.setBounds (areaCorrelacao.reduced (2, 0));
        }

        inner.removeFromTop (4);
        posicionarKnob (inner, rotuloAtraso, sliderAtraso);
    }

    // 2. Corpo & Caixa
    {
        auto inner = painelCaixa.reduced (10, 8);
        inner.removeFromTop (18);

        const int wCol = inner.getWidth() / 3;
        posicionarKnob (inner.removeFromLeft (wCol), rotuloHp,         sliderHp);
        posicionarKnob (inner.removeFromLeft (wCol), rotuloCaixaFreq,  sliderCaixaFreq);
        posicionarKnob (inner,                       rotuloCaixaGanho, sliderCaixaGanho);
    }

    // 3. Risha + Medidor de Reducao (GR)
    {
        auto inner = painelRisha.reduced (10, 8);
        inner.removeFromTop (18);

        auto areaMedidor = inner.removeFromRight (26);
        inner.removeFromRight (6);
        medidorRisha.setBounds (areaMedidor.reduced (0, 4));
        posicionarKnob (inner, rotuloRisha, sliderRisha);
    }

    // 4. Saida + Medidor de Pico (dBFS + Clip)
    {
        auto inner = painelSaida.reduced (10, 8);
        inner.removeFromTop (18);

        auto areaMedidor = inner.removeFromRight (26);
        inner.removeFromRight (6);
        medidorSaida.setBounds (areaMedidor.reduced (0, 4));
        posicionarKnob (inner, rotuloSaida, sliderSaida);
    }
}
