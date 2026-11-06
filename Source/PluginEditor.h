#pragma once
// Tape J-47 - Copyright (c) 2026 Meteora Plugins. Tutti i diritti riservati.
// Vedi LICENSE.txt. Componenti di terze parti: vedi THIRD_PARTY_NOTICES.txt.
// ===========================================================================
//  Tape J-47 - interfaccia (Meteora Plugins)
// ===========================================================================
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

// le manopole sono disegnate dall'editor: lo slider resta invisibile
class TapeInvisibleLnF : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
};

class MeteoraTapeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MeteoraTapeEditor (MeteoraTapeProcessor&);
    ~MeteoraTapeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    // bobina: faccia vista dall'alto (gira), luce della foto (ferma, si moltiplica), parti davanti (ferme)
    struct Reel
    {
        juce::Image face, light, fg;
        float cx, cy, a, b, fx, fy, fw, fh;
        juce::Image tex, lightScaled, frame;     // preparati alla grandezza della finestra
        juce::Rectangle<int> box;                // riquadro dell'ellisse in pixel del frame
    };

    void timerCallback() override;
    void prepareReel (Reel&);
    void renderReel (Reel&);
    juce::String valueText (int i) const;
    float P (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void setParam (const char* id, float plain);

    MeteoraTapeProcessor& proc;
    TapeInvisibleLnF lnf;
    juce::Image bg, bgScaled, glowImg, lofiOn, lofiOff;
    std::array<Reel, 2> reels;
    float bgK = 1.0f;
    bool sizeReady = false;
    std::array<juce::Slider, 3> knobs;          // SATURATION, MAKEUP GAIN, BIAS
    std::vector<std::unique_ptr<juce::SliderParameterAttachment>> atts;
    int lastKnob = -1, lastIps = -1, showIps = 0, showLofi = 0;
    float glow = 0.0f, tube = 0.0f, flick = 1.0f, inNeedle = -40.0f, outNeedle = -40.0f;
    float reelAngle = 0.0f, reelSpeed = 0.0f, ipsAngle = 0.0f, satRed = 0.0f;
    // SATURATION e MAKEUP GAIN collegati: alzando la saturazione l'uscita scende della stessa quantita'
    bool linkDrag = false;
    float linkIn0 = 0.0f, linkOut0 = 0.0f, prevIn = 0.0f;
    void linkOutput (float newIn);
    juce::Random rnd;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MeteoraTapeEditor)
};
