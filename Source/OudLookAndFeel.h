#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>

struct PaletaOudVisualizer
{
    juce::Colour fundoDeep;
    juce::Colour superficie;
    juce::Colour elevado;
    juce::Colour input;
    juce::Colour hover;
    juce::Colour textoPrincipal;
    juce::Colour textoSecundario;
    juce::Colour textoSuave;
    juce::Colour latao;
    juce::Colour lataoBrilho;
    juce::Colour lataoEscuro;
    juce::Colour lataoSoft;
    juce::Colour ameixa;
    juce::Colour ameixaProfunda;
    juce::Colour ameixaSoft;
    juce::Colour ameixaBorda;
    juce::Colour perigo;
    juce::Colour bordaSutil;
    juce::Colour bordaLatão;
    juce::Colour bordaForte;

    static PaletaOudVisualizer temaEbano()
    {
        return {
            juce::Colour (0xff100d0b), // --bg-deep
            juce::Colour (0xff181310), // --bg-surface
            juce::Colour (0xff201a16), // --bg-elevated
            juce::Colour (0xff251e1a), // --bg-input
            juce::Colour (0xff2d2520), // --bg-hover
            juce::Colour (0xffefe6d8), // --text-main
            juce::Colour (0xffcfc2b2), // --text-secondary
            juce::Colour (0xff9e8f7e), // --text-muted
            juce::Colour (0xffc99a49), // --accent
            juce::Colour (0xffe2b963), // --accent-bright
            juce::Colour (0xffb08030), // --accent-strong
            juce::Colour (0x24c99a49), // --accent-soft (~14%)
            juce::Colour (0xffc49be0), // --plum
            juce::Colour (0xff8755ab), // --plum-strong
            juce::Colour (0x21c49be0), // --plum-soft (~13%)
            juce::Colour (0x6bc49be0), // --plum-border (~42%)
            juce::Colour (0xffe06c6c), // --danger
            juce::Colour (0x1ae2d9c8), // --border-subtle (~10%)
            juce::Colour (0x3dc99a49), // --border-warm (~24%)
            juce::Colour (0x73c99a49)  // --border-strong (~45%)
        };
    }

    static PaletaOudVisualizer temaNaharPergaminho()
    {
        return {
            juce::Colour (0xfff4eee4), // --bg-deep (light)
            juce::Colour (0xfffcfaf6), // --bg-surface
            juce::Colour (0xfff5efe4), // --bg-elevated
            juce::Colour (0xfff1e9dc), // --bg-input
            juce::Colour (0xffe8decb), // --bg-hover
            juce::Colour (0xff261b12), // --text-main
            juce::Colour (0xff4a392a), // --text-secondary
            juce::Colour (0xff705d4a), // --text-muted
            juce::Colour (0xff8f5e1c), // --accent
            juce::Colour (0xff9a6515), // --accent-bright
            juce::Colour (0xff714812), // --accent-strong
            juce::Colour (0x1f8f5e1c), // --accent-soft (~12%)
            juce::Colour (0xff6b3f8c), // --plum
            juce::Colour (0xff532e70), // --plum-strong
            juce::Colour (0x1a6b3f8c), // --plum-soft (~10%)
            juce::Colour (0x5c6b3f8c), // --plum-border (~36%)
            juce::Colour (0xffb92b2b), // --danger
            juce::Colour (0x1f4a392a), // --border-subtle (~12%)
            juce::Colour (0x408f5e1c), // --border-warm (~25%)
            juce::Colour (0x7a8f5e1c)  // --border-strong (~48%)
        };
    }
};

class OudLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit OudLookAndFeel (bool usarTemaClaroInicial = false)
        : temaClaro (usarTemaClaroInicial)
    {
        aplicarTema (temaClaro);
    }

    void aplicarTema (bool claro)
    {
        temaClaro = claro;
        paleta = claro ? PaletaOudVisualizer::temaNaharPergaminho()
                       : PaletaOudVisualizer::temaEbano();

        setColour (juce::Slider::textBoxTextColourId, paleta.textoPrincipal);
        setColour (juce::Slider::textBoxBackgroundColourId, paleta.input);
        setColour (juce::Slider::textBoxOutlineColourId, paleta.bordaSutil);
        setColour (juce::Slider::textBoxHighlightColourId, paleta.lataoSoft);

        setColour (juce::ComboBox::backgroundColourId, paleta.input);
        setColour (juce::ComboBox::textColourId, paleta.textoPrincipal);
        setColour (juce::ComboBox::outlineColourId, paleta.bordaSutil);
        setColour (juce::ComboBox::arrowColourId, paleta.latao);

        setColour (juce::PopupMenu::backgroundColourId, paleta.elevado);
        setColour (juce::PopupMenu::textColourId, paleta.textoPrincipal);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, paleta.lataoSoft);
        setColour (juce::PopupMenu::highlightedTextColourId, paleta.lataoBrilho);

        setColour (juce::Label::textColourId, paleta.textoSuave);
    }

    bool estaTemaClaro() const noexcept { return temaClaro; }
    const PaletaOudVisualizer& getPaleta() const noexcept { return paleta; }

    juce::Font getFonteTitulo (float altura) const
    {
        return juce::Font (juce::FontOptions ("Cormorant Garamond", altura, juce::Font::bold));
    }

    juce::Font getFonteInterface (float altura, bool negrito = false) const
    {
        const int estilo = negrito ? juce::Font::bold : juce::Font::plain;
        return juce::Font (juce::FontOptions ("Plus Jakarta Sans", altura, estilo));
    }

    void drawRotarySlider (juce::Graphics& g,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          const float rotaryStartAngle,
                          const float rotaryEndAngle,
                          juce::Slider&) override
    {
        const float diametro = static_cast<float> (std::min (width, height)) - 6.0f;
        if (diametro <= 10.0f)
            return;

        const float raio = diametro * 0.5f;
        const float cx = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
        const float cy = static_cast<float> (y) + static_cast<float> (height) * 0.5f;
        const float anguloAtual = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const float espessuraTrilha = 4.2f;
        const float raioArco = raio - espessuraTrilha * 0.5f;

        // Trilha de fundo em hover/input
        juce::Path arcoBase;
        arcoBase.addCentredArc (cx, cy, raioArco, raioArco,
                              0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (paleta.hover);
        g.strokePath (arcoBase, juce::PathStrokeType (espessuraTrilha,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));

        // Arco ativo em latao
        if (sliderPosProportional > 0.001f)
        {
            juce::Path arcoAtivo;
            arcoAtivo.addCentredArc (cx, cy, raioArco, raioArco,
                                   0.0f, rotaryStartAngle, anguloAtual, true);
            g.setColour (paleta.latao);
            g.strokePath (arcoAtivo, juce::PathStrokeType (espessuraTrilha,
                                                       juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
        }

        // Disco interno elevado com borda em pergaminho/latao
        const float raioMiolo = raio * 0.65f;
        g.setColour (paleta.elevado);
        g.fillEllipse (cx - raioMiolo, cy - raioMiolo, raioMiolo * 2.0f, raioMiolo * 2.0f);

        g.setColour (paleta.bordaLatão);
        g.drawEllipse (cx - raioMiolo, cy - raioMiolo, raioMiolo * 2.0f, raioMiolo * 2.0f, 1.15f);

        // Indicador radial em latao brilhante
        juce::Path ponteiro;
        const float rIni = raioMiolo * 0.24f;
        const float rFim = raioMiolo * 0.88f;
        ponteiro.startNewSubPath (cx + rIni * std::sin (anguloAtual),
                                cy - rIni * std::cos (anguloAtual));
        ponteiro.lineTo (cx + rFim * std::sin (anguloAtual),
                         cy - rFim * std::cos (anguloAtual));

        g.setColour (paleta.lataoBrilho);
        g.strokePath (ponteiro, juce::PathStrokeType (2.3f,
                                                    juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    void drawToggleButton (juce::Graphics& g,
                          juce::ToggleButton& button,
                          bool /*highlighted*/,
                          bool /*down*/) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool ativo = button.getToggleState();

        // Botao de inversao de fase usa destaque em latao/ameixa quando ativo
        g.setColour (ativo ? paleta.lataoSoft : paleta.input);
        g.fillRoundedRectangle (bounds, 8.0f);

        g.setColour (ativo ? paleta.latao : paleta.bordaSutil);
        g.drawRoundedRectangle (bounds, 8.0f, 1.25f);

        g.setColour (ativo ? paleta.lataoBrilho : paleta.textoPrincipal);
        g.setFont (getFonteInterface (12.5f, true));
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().reduced (8, 2),
                          juce::Justification::centred, 1);
    }

    void drawButtonBackground (juce::Graphics& g,
                               juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        if (shouldDrawButtonAsDown)
            bounds = bounds.translated (0.0f, 1.0f);

        juce::Colour fundo = paleta.input;
        juce::Colour borda = paleta.bordaSutil;

        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
        {
            fundo = paleta.hover;
            borda = paleta.bordaLatão;
        }

        g.setColour (fundo);
        g.fillRoundedRectangle (bounds, 8.0f);

        g.setColour (borda);
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);
    }

    void drawButtonText (juce::Graphics& g,
                         juce::TextButton& button,
                         bool /*highlighted*/,
                         bool /*down*/) override
    {
        g.setFont (getFonteInterface (12.0f, true));
        g.setColour (paleta.textoSecundario);
        g.drawFittedText (button.getButtonText(),
                          button.getLocalBounds().reduced (6, 2),
                          juce::Justification::centred, 1);
    }

    void drawComboBox (juce::Graphics& g, int width, int height,
                       bool /*isButtonDown*/, int /*buttonX*/, int /*buttonY*/,
                       int /*buttonW*/, int /*buttonH*/, juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                   static_cast<float> (width),
                                                   static_cast<float> (height)).reduced (0.5f);

        g.setColour (paleta.input);
        g.fillRoundedRectangle (bounds, 8.0f);

        g.setColour (box.hasKeyboardFocus (false) ? paleta.bordaForte : paleta.bordaSutil);
        g.drawRoundedRectangle (bounds, 8.0f, 1.0f);

        // Seta em latao no canto direito
        const float centroX = static_cast<float> (width) - 14.0f;
        const float centroY = static_cast<float> (height) * 0.5f;
        juce::Path seta;
        seta.startNewSubPath (centroX - 4.0f, centroY - 2.0f);
        seta.lineTo (centroX + 4.0f, centroY - 2.0f);
        seta.lineTo (centroX, centroY + 3.0f);
        seta.closeSubPath();

        g.setColour (paleta.latao);
        g.fillPath (seta);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 1, box.getWidth() - 28, box.getHeight() - 2);
        label.setFont (getFonteInterface (12.5f, true));
        label.setColour (juce::Label::textColourId, paleta.textoPrincipal);
    }

private:
    bool temaClaro { false };
    PaletaOudVisualizer paleta { PaletaOudVisualizer::temaEbano() };
};
