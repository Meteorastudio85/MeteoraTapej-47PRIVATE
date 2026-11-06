// Tape J-47 - Copyright (c) 2026 Meteora Plugins. Tutti i diritti riservati.
// Vedi LICENSE.txt. Componenti di terze parti: vedi THIRD_PARTY_NOTICES.txt.
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

MeteoraTapeProcessor::MeteoraTapeProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout MeteoraTapeProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    // INPUT GAIN: fino a +24 dB per spingere il nastro in saturazione (0 dB a meta' corsa, sul 5)
    juce::NormalisableRange<float> inR (-12.0f, 24.0f); inR.setSkewForCentre (0.0f);
    // OUTPUT: make-up, fino a -24 dB per compensare l'input spinto (0 dB a meta' corsa)
    juce::NormalisableRange<float> outR (-24.0f, 12.0f); outR.setSkewForCentre (0.0f);
    // gli ID dei parametri sono gli stessi della versione precedente: i vecchi progetti si riaprono uguali.
    // BIAS e LO-FI sono nuovi e partono neutri (0 e spento), quindi non cambiano il suono dei vecchi progetti.
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "in", 1 },   "Saturation",   inR, 0.0f));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 },  "Dry/Wet",      juce::NormalisableRange<float> (0.0f, 100.0f), 100.0f));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "out", 1 },  "Makeup Gain",  outR, 0.0f));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "hiss", 1 }, "Hiss",         juce::NormalisableRange<float> (0.0f, 100.0f), 0.0f));
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "ips", 1 }, "Tape Speed", juce::StringArray { "7.5 IPS", "15 IPS", "30 IPS" }, 1));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "bias", 1 }, "Bias",         juce::NormalisableRange<float> (-100.0f, 100.0f), 0.0f));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "lofi", 1 },  "Lo-Fi",        false));
    return l;
}

bool MeteoraTapeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto& out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono())
           && out == l.getMainInputChannelSet();
}

void MeteoraTapeProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    engine.prepare (sr);
    lofiSm = P ("lofi") > 0.5f ? 1.0 : 0.0;
    setLatencySamples (engine.latencySamples());
}

void MeteoraTapeProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    bypassed.store (false);
    lastProcessMs.store (juce::Time::getMillisecondCounter());
    const int n = buf.getNumSamples();
    const int nch = juce::jmin (2, buf.getNumChannels());
    if (nch == 0 || n == 0) return;

    // LO-FI entra ed esce in circa 80 ms (filtri e oscillazione senza click)
    {
        const double target = P ("lofi") > 0.5f ? 1.0 : 0.0;
        const double step = (double) buf.getNumSamples() / (0.08 * sr);
        lofiSm += juce::jlimit (-step, step, target - lofiSm);
    }
    engine.setParams (P ("in"), P ("out"), P ("mix") / 100.0, P ("hiss"), (int) std::lround (P ("ips")),
                      P ("bias") / 100.0, lofiSm);
    double inS = 0, outS = 0, inP = 0, outP = 0;

    float* l = buf.getWritePointer (0);
    float* r = nch > 1 ? buf.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        float x[2] = { l[i], r != nullptr ? r[i] : l[i] };
        engine.process (x, 1.0);
        inS += engine.inSq; outS += engine.outSq;
        inP = juce::jmax (inP, engine.inPk); outP = juce::jmax (outP, engine.outPk);
        l[i] = x[0];
        if (r != nullptr) r[i] = x[1];
    }
    // VU: 0 VU = -18 dBFS RMS
    inVu.store ((float) (10.0 * std::log10 (juce::jmax (inS / n, 1.0e-12)) + 18.0));
    outVu.store ((float) (10.0 * std::log10 (juce::jmax (outS / n, 1.0e-12)) + 18.0));
    inPeakDb.store ((float) (20.0 * std::log10 (juce::jmax (inP, 1.0e-6))));
    outPeakDb.store ((float) (20.0 * std::log10 (juce::jmax (outP, 1.0e-6))));
}

juce::AudioProcessorEditor* MeteoraTapeProcessor::createEditor() { return new MeteoraTapeEditor (*this); }

void MeteoraTapeProcessor::getStateInformation (juce::MemoryBlock& d)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, d);
}

void MeteoraTapeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MeteoraTapeProcessor(); }
