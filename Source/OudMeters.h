#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "OudDspChain.h"
#include <algorithm>
#include <cmath>

class MedidorReducaoRisha : public juce::Component,
                            public juce::SettableTooltipClient
{
public:
    MedidorReducaoRisha()
    {
        setTooltip (juce::String::fromUTF8 ("Redu" "\xc3\xa7\xc3\xa3" "o de transiente da Risha (dB)"));
    }

    void setTemaClaro (bool claro)
    {
        if (temaClaro != claro)
        {
            temaClaro = claro;
            repaint();
        }
    }

    void atualizarLeitura (float novaReducaoDb)
    {
        const float limitada = juce::jlimit (0.0f, maxEscalaDb, novaReducaoDb);

        if (limitada >= exibidoDb)
            exibidoDb = limitada;
        else
            exibidoDb = std::max (0.0f, exibidoDb - decaimentoPorQuadroDb);

        if (std::abs (exibidoDb - ultimoDesenhadoDb) > 0.04f)
        {
            ultimoDesenhadoDb = exibidoDb;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto areaTexto = area.removeFromBottom (16.0f);
        auto barra = area.reduced (2.0f, 1.0f);

        const auto fundoBarra  = temaClaro ? juce::Colour (0xffe6dfd2) : juce::Colour (0xff120e0c);
        const auto bordaBarra  = temaClaro ? juce::Colour (0xffd2c7b6) : juce::Colour (0xff2b221c);
        const auto corAmeixa   = temaClaro ? juce::Colour (0xff6b3f8c) : juce::Colour (0xffc49be0);
        const auto corAmeixaHi = temaClaro ? juce::Colour (0xff8855ad) : juce::Colour (0xffdfb9f7);
        const auto corTexto    = temaClaro ? juce::Colour (0xff5c4b3a) : juce::Colour (0xffb7a997);

        g.setColour (fundoBarra);
        g.fillRoundedRectangle (barra, 3.0f);

        const float proporcao = juce::jlimit (0.0f, 1.0f, exibidoDb / maxEscalaDb);
        if (proporcao > 0.005f)
        {
            auto interna = barra.reduced (1.5f);
            const float h = interna.getHeight() * proporcao;
            auto retPreenchido = interna.withHeight (h);

            juce::ColourGradient grad (
                corAmeixaHi, retPreenchido.getCentreX(), retPreenchido.getY(),
                corAmeixa,   retPreenchido.getCentreX(), retPreenchido.getBottom(),
                false);

            g.setGradientFill (grad);
            g.fillRoundedRectangle (retPreenchido, 2.0f);
        }

        g.setColour (bordaBarra.withAlpha (0.85f));
        for (float marcaDb : { 3.0f, 6.0f, 9.0f })
        {
            const float y = barra.getY() + barra.getHeight() * (marcaDb / maxEscalaDb);
            g.drawHorizontalLine (static_cast<int> (std::round (y)), barra.getX() + 1.0f, barra.getRight() - 1.0f);
        }

        g.setColour (bordaBarra);
        g.drawRoundedRectangle (barra, 3.0f, 1.0f);

        g.setColour (exibidoDb > 0.2f ? corAmeixa : corTexto);
        g.setFont (juce::Font (juce::FontOptions (9.5f)));
        const juce::String texto = (exibidoDb < 0.1f)
            ? "0.0"
            : ("-" + juce::String (exibidoDb, 1));
        g.drawFittedText (texto, areaTexto.toNearestInt(), juce::Justification::centred, 1);
    }

private:
    static constexpr float maxEscalaDb           = 12.0f;
    static constexpr float decaimentoPorQuadroDb = 0.55f;

    bool temaClaro          = false;
    float exibidoDb         = 0.0f;
    float ultimoDesenhadoDb = -1.0f;
};

class MedidorPicoSaida : public juce::Component,
                         public juce::SettableTooltipClient
{
public:
    MedidorPicoSaida()
    {
        setTooltip (juce::String::fromUTF8 ("Pico de sa" "\xc3\xad" "da (dBFS) - clique para limpar indicador de clip"));
    }

    void setTemaClaro (bool claro)
    {
        if (temaClaro != claro)
        {
            temaClaro = claro;
            repaint();
        }
    }

    void atualizarLeitura (float picoLinear)
    {
        const float novoDb = juce::Decibels::gainToDecibels (picoLinear, minDb);

        if (novoDb >= exibidoDb)
            exibidoDb = novoDb;
        else
            exibidoDb = std::max (minDb, exibidoDb - decaimentoPorQuadroDb);

        if (novoDb >= -0.1f)
            emClip = true;

        if (novoDb >= picoHoldDb)
        {
            picoHoldDb = novoDb;
            quadrosHoldRestantes = 35;
        }
        else if (quadrosHoldRestantes > 0)
        {
            --quadrosHoldRestantes;
        }
        else
        {
            picoHoldDb = std::max (minDb, picoHoldDb - 0.9f);
        }

        const bool mudouBarra = std::abs (exibidoDb - ultimoDesenhadoDb) > 0.08f;
        const bool mudouHold  = std::abs (picoHoldDb - ultimoHoldDesenhadoDb) > 0.08f;
        const bool mudouClip  = (emClip != ultimoClipDesenhado);

        if (mudouBarra || mudouHold || mudouClip)
        {
            ultimoDesenhadoDb     = exibidoDb;
            ultimoHoldDesenhadoDb = picoHoldDb;
            ultimoClipDesenhado   = emClip;
            repaint();
        }
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        emClip = false;
        picoHoldDb = exibidoDb;
        quadrosHoldRestantes = 0;
        ultimoClipDesenhado = true;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        auto areaTexto = area.removeFromBottom (16.0f);
        auto ledClip   = area.removeFromTop (6.0f).reduced (2.0f, 1.0f);
        area.removeFromTop (2.0f);
        auto barra = area.reduced (2.0f, 0.0f);

        const auto fundoBarra  = temaClaro ? juce::Colour (0xffe6dfd2) : juce::Colour (0xff120e0c);
        const auto bordaBarra  = temaClaro ? juce::Colour (0xffd2c7b6) : juce::Colour (0xff2b221c);
        const auto corLatao    = temaClaro ? juce::Colour (0xff8f5e1c) : juce::Colour (0xffc99a49);
        const auto corLataoHi  = temaClaro ? juce::Colour (0xffb57d2c) : juce::Colour (0xffe2b963);
        const auto corClip     = temaClaro ? juce::Colour (0xffa83b2d) : juce::Colour (0xffc95443);
        const auto corTexto    = temaClaro ? juce::Colour (0xff5c4b3a) : juce::Colour (0xffb7a997);

        g.setColour (emClip ? corClip : fundoBarra);
        g.fillRoundedRectangle (ledClip, 2.0f);
        g.setColour (bordaBarra);
        g.drawRoundedRectangle (ledClip, 2.0f, 0.8f);

        g.setColour (fundoBarra);
        g.fillRoundedRectangle (barra, 3.0f);

        auto dbParaProporcao = [] (float db) noexcept
        {
            const float clamped = juce::jlimit (minDb, maxDb, db);
            const float norm = (clamped - minDb) / (maxDb - minDb);
            return std::pow (norm, 1.35f);
        };

        auto interna = barra.reduced (1.5f);

        const float prop = dbParaProporcao (exibidoDb);
        if (prop > 0.004f)
        {
            const float h = interna.getHeight() * prop;
            auto retPreenchido = interna.withTop (interna.getBottom() - h);

            juce::ColourGradient grad (
                corLataoHi, retPreenchido.getCentreX(), interna.getY(),
                corLatao,   retPreenchido.getCentreX(), interna.getBottom(),
                false);

            if (exibidoDb > -3.0f)
                grad.addColour (0.12, corClip);

            g.setGradientFill (grad);
            g.fillRoundedRectangle (retPreenchido, 2.0f);
        }

        if (picoHoldDb > minDb + 1.0f)
        {
            const float propHold = dbParaProporcao (picoHoldDb);
            const float yHold = interna.getBottom() - (interna.getHeight() * propHold);
            g.setColour (picoHoldDb >= -0.1f ? corClip : corLataoHi);
            g.fillRect (interna.getX(), std::round (yHold) - 1.0f, interna.getWidth(), 2.0f);
        }

        g.setColour (bordaBarra.withAlpha (0.85f));
        for (float marcaDb : { -6.0f, -12.0f, -24.0f })
        {
            const float y = interna.getBottom() - (interna.getHeight() * dbParaProporcao (marcaDb));
            g.drawHorizontalLine (static_cast<int> (std::round (y)), barra.getX() + 1.0f, barra.getRight() - 1.0f);
        }

        g.setColour (bordaBarra);
        g.drawRoundedRectangle (barra, 3.0f, 1.0f);

        g.setColour (emClip ? corClip : corTexto);
        g.setFont (juce::Font (juce::FontOptions (9.5f)));
        const juce::String texto = (picoHoldDb <= minDb + 0.5f)
            ? "-inf"
            : juce::String (picoHoldDb, 1);
        g.drawFittedText (texto, areaTexto.toNearestInt(), juce::Justification::centred, 1);
    }

private:
    static constexpr float minDb                 = -60.0f;
    static constexpr float maxDb                 = 3.0f;
    static constexpr float decaimentoPorQuadroDb = 1.2f;

    bool temaClaro              = false;
    bool emClip                 = false;
    bool ultimoClipDesenhado    = false;
    int quadrosHoldRestantes    = 0;
    float exibidoDb             = minDb;
    float picoHoldDb            = minDb;
    float ultimoDesenhadoDb     = minDb - 1.0f;
    float ultimoHoldDesenhadoDb = minDb - 1.0f;
};

class MedidorCorrelacaoFase : public juce::Component,
                              public juce::SettableTooltipClient
{
public:
    MedidorCorrelacaoFase()
    {
        setTooltip (juce::String::fromUTF8 (
            "Correla" "\xc3\xa7\xc3\xa3" "o de fase entre Canal 1 e Canal 2 (+1 em fase, -1 fora de fase)"));
    }

    void setTemaClaro (bool claro)
    {
        if (temaClaro != claro)
        {
            temaClaro = claro;
            repaint();
        }
    }

    void atualizarLeitura (const LeituraCorrelacaoOud& leitura)
    {
        estadoAtual = leitura.estado;

        const float alvo = (estadoAtual == EstadoCorrelacao::Ativo)
            ? juce::jlimit (-1.0f, 1.0f, leitura.valor)
            : 0.0f;

        const float fator = (estadoAtual == EstadoCorrelacao::Ativo) ? 0.35f : 0.20f;
        valorExibido += (alvo - valorExibido) * fator;
        if (std::abs (valorExibido) < 0.002f && alvo == 0.0f)
            valorExibido = 0.0f;

        const bool mudouEstado = (estadoAtual != ultimoEstadoDesenhado);
        const bool mudouValor  = std::abs (valorExibido - ultimoValorDesenhado) > 0.005f;

        if (mudouEstado || mudouValor)
        {
            ultimoEstadoDesenhado = estadoAtual;
            ultimoValorDesenhado  = valorExibido;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();

        const auto fundoBarra = temaClaro ? juce::Colour (0xffe6dfd2) : juce::Colour (0xff120e0c);
        const auto bordaBarra = temaClaro ? juce::Colour (0xffd2c7b6) : juce::Colour (0xff2b221c);
        const auto corLatao   = temaClaro ? juce::Colour (0xff8f5e1c) : juce::Colour (0xffc99a49);
        const auto corLataoHi = temaClaro ? juce::Colour (0xffb57d2c) : juce::Colour (0xffe2b963);
        const auto corNeg     = temaClaro ? juce::Colour (0xffa83b2d) : juce::Colour (0xffc95443);
        const auto corTexto   = temaClaro ? juce::Colour (0xff5c4b3a) : juce::Colour (0xffb7a997);

        auto linhaBarra  = area.removeFromTop (13.0f);
        auto areaLeitura = linhaBarra.removeFromRight (40.0f);
        linhaBarra.removeFromRight (4.0f);
        auto barra       = linhaBarra.reduced (0.0f, 1.0f);
        auto areaEscala  = area.withWidth (barra.getWidth());

        g.setColour (fundoBarra);
        g.fillRoundedRectangle (barra, 3.0f);

        auto interna = barra.reduced (1.5f);
        const float centroX = interna.getCentreX();
        const float meiaLargura = interna.getWidth() * 0.5f;

        if (estadoAtual == EstadoCorrelacao::Ativo || std::abs (valorExibido) > 0.01f)
        {
            const float deslocamento = valorExibido * meiaLargura;
            const bool positivo = valorExibido >= 0.0f;

            const float x0 = positivo ? centroX : (centroX + deslocamento);
            const float w  = std::abs (deslocamento);

            if (w > 0.8f)
            {
                juce::Rectangle<float> preenchimento (x0, interna.getY(), w, interna.getHeight());
                if (positivo)
                {
                    juce::ColourGradient grad (
                        corLatao,   centroX, interna.getCentreY(),
                        corLataoHi, interna.getRight(), interna.getCentreY(),
                        false);
                    g.setGradientFill (grad);
                }
                else
                {
                    g.setColour (corNeg);
                }
                g.fillRoundedRectangle (preenchimento, 1.8f);
            }

            const float pontaX = juce::jlimit (interna.getX(), interna.getRight() - 2.0f, centroX + deslocamento - 1.0f);
            g.setColour (positivo ? corLataoHi : corNeg.brighter (0.15f));
            g.fillRect (pontaX, interna.getY(), 2.0f, interna.getHeight());
        }

        g.setColour (bordaBarra.brighter (0.18f));
        g.drawVerticalLine (static_cast<int> (std::round (centroX)), barra.getY() + 1.0f, barra.getBottom() - 1.0f);

        g.setColour (bordaBarra);
        g.drawRoundedRectangle (barra, 3.0f, 1.0f);

        g.setColour (corTexto.withAlpha (0.85f));
        g.setFont (juce::Font (juce::FontOptions (8.5f)));
        const auto escInt = areaEscala.toNearestInt();
        g.drawText ("-1", escInt, juce::Justification::centredLeft, false);
        g.drawText ("0",  escInt, juce::Justification::centred, false);
        g.drawText ("+1", escInt, juce::Justification::centredRight, false);

        juce::String texto = "---";
        juce::Colour corValor = corTexto;

        if (estadoAtual == EstadoCorrelacao::Mono)
        {
            texto = "MONO";
        }
        else if (estadoAtual == EstadoCorrelacao::Ativo)
        {
            const juce::String sinal = (valorExibido > 0.005f) ? "+" : "";
            texto = sinal + juce::String (valorExibido, 2);
            corValor = (valorExibido < -0.05f) ? corNeg : (valorExibido > 0.25f ? corLatao : corTexto);
        }

        g.setColour (corValor);
        g.setFont (juce::Font (juce::FontOptions (9.5f)));
        g.drawFittedText (texto, areaLeitura.toNearestInt(), juce::Justification::centredRight, 1);
    }

private:
    bool temaClaro = false;
    EstadoCorrelacao estadoAtual           = EstadoCorrelacao::Silencio;
    EstadoCorrelacao ultimoEstadoDesenhado = EstadoCorrelacao::Ativo;
    float valorExibido                     = 0.0f;
    float ultimoValorDesenhado             = -2.0f;
};
