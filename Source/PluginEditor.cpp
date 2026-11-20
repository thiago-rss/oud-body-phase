#include "PluginEditor.h"

OudBodyPhaseAudioProcessorEditor::OudBodyPhaseAudioProcessorEditor (OudBodyPhaseAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&oudLookAndFeel);

    botaoTema.setTooltip ("Alternar entre tema Ebano e tema claro Nahar");
    botaoTema.onClick = [this] { alternarTema(); };
    addAndMakeVisible (botaoTema);

    addAndMakeVisible (botaoFase);

    configurarRotulo (rotuloAlvoFase, "Alvo");
    caixaAlvoFase.addItemList ({ "Ambos", "Canal 1", "Canal 2" }, 1);
    addAndMakeVisible (caixaAlvoFase);

    configurarRotulo (rotuloAtraso, "Atraso");
    configurarRotulo (rotuloHp, "HPF");
    configurarRotulo (rotuloCaixaFreq, "Freq Caixa");
    configurarRotulo (rotuloCaixaGanho, "Ganho Caixa");
    configurarRotulo (rotuloRisha, "Suavizar Risha");
    configurarRotulo (rotuloSaida, "Ganho Saida");

    configurarKnob (sliderAtraso);
    configurarKnob (sliderHp);
    configurarKnob (sliderCaixaFreq);
    configurarKnob (sliderCaixaGanho);
    configurarKnob (sliderRisha);
    configurarKnob (sliderSaida);

    attachFase       = std::make_unique<ButtonAttachment>   (audioProcessor.apvts, OudParamIDs::inverterFase,  botaoFase);
    attachAlvoFase   = std::make_unique<ComboBoxAttachment> (audioProcessor.apvts, OudParamIDs::alvoFase,      caixaAlvoFase);
    attachAtraso     = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::atrasoFinoMs,  sliderAtraso);
    attachHp         = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::freqCorteHpHz, sliderHp);
    attachCaixaFreq  = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::freqCaixaHz,   sliderCaixaFreq);
    attachCaixaGanho = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::ganhoCaixaDb,  sliderCaixaGanho);
    attachRisha      = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::suavizarRisha, sliderRisha);
    attachSaida      = std::make_unique<SliderAttachment>   (audioProcessor.apvts, OudParamIDs::ganhoSaidaDb,  sliderSaida);

    aplicarCoresComponentes();
    setSize (800, 336);
}

OudBodyPhaseAudioProcessorEditor::~OudBodyPhaseAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void OudBodyPhaseAudioProcessorEditor::alternarTema()
{
    const bool proximoClaro = ! oudLookAndFeel.estaTemaClaro();
    oudLookAndFeel.aplicarTema (proximoClaro);
    botaoTema.setButtonText (proximoClaro ? "Ebano" : "Nahar");
    aplicarCoresComponentes();
    repaint();
}

void OudBodyPhaseAudioProcessorEditor::aplicarCoresComponentes()
{
    const auto& pal = oudLookAndFeel.getPaleta();

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

    // Fundo em gradiente sutil no mesmo espirito de --bg-page-grad
    juce::ColourGradient gradienteFundo (
        pal.fundoDeep.brighter (0.04f), 0.0f, 0.0f,
        pal.fundoDeep.darker (0.12f), 0.0f, static_cast<float> (getHeight()), false);
    g.setGradientFill (gradienteFundo);
    g.fillAll();

    // Cartao de cabecalho estilo .studio-header
    if (! cartaoCabecalho.isEmpty())
    {
        const auto cabF = cartaoCabecalho.toFloat();
        g.setColour (pal.superficie);
        g.fillRoundedRectangle (cabF, 14.0f);
        g.setColour (pal.bordaSutil);
        g.drawRoundedRectangle (cabF, 14.0f, 1.0f);

        auto interno = cartaoCabecalho.reduced (14, 6);
        auto direita = interno.removeFromRight (88);
        botaoTema.setBounds (direita.withSizeKeepingCentre (76, 28));

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
    desenharCartaoSecao (g, painelSaida, "Saida",              pal.latao);
}

void OudBodyPhaseAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (14, 12);

    cartaoCabecalho = area.removeFromTop (46);
    area.removeFromTop (10);

    const int gap = 10;
    const int larguraTotal = area.getWidth();

    const int wFase  = static_cast<int> (larguraTotal * 0.30f);
    const int wCaixa = static_cast<int> (larguraTotal * 0.38f);
    const int wRisha = static_cast<int> (larguraTotal * 0.16f);
    const int wSaida = std::max (84, larguraTotal - wFase - wCaixa - wRisha - gap * 3);

    painelFase  = area.removeFromLeft (wFase);
    area.removeFromLeft (gap);
    painelCaixa = area.removeFromLeft (wCaixa);
    area.removeFromLeft (gap);
    painelRisha = area.removeFromLeft (wRisha);
    area.removeFromLeft (gap);
    painelSaida = area;

    // 1. Fase & Alinhamento
    {
        auto inner = painelFase.reduced (10, 8);
        inner.removeFromTop (18);

        auto linhaTopo = inner.removeFromTop (30);
        botaoFase.setBounds (linhaTopo.removeFromLeft (142));
        linhaTopo.removeFromLeft (8);
        rotuloAlvoFase.setBounds (linhaTopo.removeFromLeft (30));
        caixaAlvoFase.setBounds (linhaTopo);

        inner.removeFromTop (6);
        posicionarKnob (inner, rotuloAtraso, sliderAtraso);
    }

    // 2. Corpo & Caixa
    {
        auto inner = painelCaixa.reduced (10, 8);
        inner.removeFromTop (18);

        const int wCol = inner.getWidth() / 3;
        posicionarKnob (inner.removeFromLeft (wCol), rotuloHp,         sliderHp);
        posicionarKnob (inner.removeFromLeft (wCol), rotuloCaixaFreq,  sliderCaixaFreq);
        posicionarKnob (inner,                        rotuloCaixaGanho, sliderCaixaGanho);
    }

    // 3. Risha
    {
        auto inner = painelRisha.reduced (10, 8);
        inner.removeFromTop (18);
        posicionarKnob (inner, rotuloRisha, sliderRisha);
    }

    // 4. Saida
    {
        auto inner = painelSaida.reduced (10, 8);
        inner.removeFromTop (18);
        posicionarKnob (inner, rotuloSaida, sliderSaida);
    }
}
