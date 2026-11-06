// Tape J-47 - Copyright (c) 2026 Meteora Plugins. Tutti i diritti riservati.
// Vedi LICENSE.txt. Componenti di terze parti: vedi THIRD_PARTY_NOTICES.txt.
#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace
{
    constexpr float kW = 1376.0f, kH = 768.0f;
    constexpr int kDefaultWidth = 1080;
    constexpr const char* kSizeKey = "uiWidthV2";

    // pomelli (lo slider invisibile copre il pomello): SATURATION, MAKEUP GAIN, BIAS
    struct TapeKnob { const char* id; const char* name; float cx, cy, r; };
    const TapeKnob kKnobs[3] = {
        { "in",   "Saturation",   708.0f, 576.0f, 62.0f },
        { "out",  "Makeup Gain",  935.0f, 575.0f, 68.0f },
        { "bias", "Bias",        1112.0f, 690.0f, 32.0f },
    };
    // SATURATION: puntino chiaro sulla gonna, da "0" a "12"
    constexpr float kSatX = 708.0f, kSatY = 576.0f, kSatR = 59.5f, kSatA0 = -148.0f, kSatA1 = 138.0f;
    // MAKEUP GAIN: riga scura sulla faccia
    constexpr float kOutX = 941.0f, kOutY = 596.0f, kOutR = 56.0f, kOutA0 = -135.0f, kOutA1 = 135.0f;
    // BIAS: riga scura sulla faccia, da UNDER a OVER
    constexpr float kBiasX = 1114.0f, kBiasY = 691.0f, kBiasR = 26.0f;
    // TAPE SPEED: il pomello a becco ruota sui tre scatti
    constexpr float kSelX = 1168.0f, kSelY = 556.0f, kSelR = 56.0f;
    const float kSelAngle[3] = { -47.0f, 0.0f, 46.0f };
    // LO-FI: levetta (giu' = spento, su = acceso)
    constexpr float kLofiX = 1205.0f, kLofiY = 640.0f, kLofiW = 70.0f, kLofiH = 80.0f;
    // VU: perno e centro del quadrante (0 VU = -18 dBFS)
    const float kVuX[2] = { 230.0f, 472.0f };
    const float kVuY[2] = { 587.0f, 584.0f };
    // logo M (si accende come una valvola) e lampadine dei canali in alto
    constexpr float kLogoX = 667.5f, kLogoY = 676.0f, kLogoS = 88.5f;
    constexpr float kLampY = 65.0f;
}

MeteoraTapeEditor::MeteoraTapeEditor (MeteoraTapeProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    bg = juce::ImageCache::getFromMemory (BinaryData::ui_jpg, BinaryData::ui_jpgSize);
    glowImg = juce::ImageCache::getFromMemory (BinaryData::logo_glow_png, BinaryData::logo_glow_pngSize);
    lofiOn  = juce::ImageCache::getFromMemory (BinaryData::lofi_on_png,  BinaryData::lofi_on_pngSize);
    lofiOff = juce::ImageCache::getFromMemory (BinaryData::lofi_off_png, BinaryData::lofi_off_pngSize);
    reels[0].face  = juce::ImageCache::getFromMemory (BinaryData::reelL_face_jpg,  BinaryData::reelL_face_jpgSize);
    reels[0].light = juce::ImageCache::getFromMemory (BinaryData::reelL_light_png, BinaryData::reelL_light_pngSize);
    reels[0].fg    = juce::ImageCache::getFromMemory (BinaryData::fgL_png,         BinaryData::fgL_pngSize);
    reels[0].cx = 398.75f; reels[0].cy = 193.0f; reels[0].a = 250.0f; reels[0].b = 92.0f;
    reels[0].fx = 231.0f; reels[0].fy = 96.5f; reels[0].fw = 390.0f; reels[0].fh = 193.5f;
    reels[1].face  = juce::ImageCache::getFromMemory (BinaryData::reelR_face_jpg,  BinaryData::reelR_face_jpgSize);
    reels[1].light = juce::ImageCache::getFromMemory (BinaryData::reelR_light_png, BinaryData::reelR_light_pngSize);
    reels[1].fg    = juce::ImageCache::getFromMemory (BinaryData::fgR_png,         BinaryData::fgR_pngSize);
    reels[1].cx = 978.5f; reels[1].cy = 191.5f; reels[1].a = 248.0f; reels[1].b = 90.5f;
    reels[1].fx = 762.0f; reels[1].fy = 96.5f; reels[1].fw = 384.0f; reels[1].fh = 190.5f;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& s = knobs[i];
        s.setLookAndFeel (&lnf);
        s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setMouseDragSensitivity (240);
        addAndMakeVisible (s);
        atts.push_back (std::make_unique<juce::SliderParameterAttachment> (*proc.apvts.getParameter (kKnobs[i].id), s));
        s.onValueChange = [this, i]
        {
            lastKnob = (int) i; showIps = 0; showLofi = 0;
            if (i == 0) linkOutput ((float) knobs[0].getValue());
            repaint();
        };
    }
    knobs[2].setDoubleClickReturnValue (true, 0.0);          // doppio clic su BIAS = taratura normale
    knobs[0].onDragStart = [this]
    {
        linkDrag = true; linkIn0 = P ("in"); linkOut0 = P ("out");
        if (auto* o = proc.apvts.getParameter ("out")) o->beginChangeGesture();
    };
    knobs[0].onDragEnd = [this]
    {
        linkDrag = false;
        if (auto* o = proc.apvts.getParameter ("out")) o->endChangeGesture();
    };
    prevIn = P ("in");
    lastIps = juce::jlimit (0, 2, (int) std::lround (P ("ips")));
    ipsAngle = kSelAngle[lastIps];

    const int w = juce::jlimit (688, 2064, (int) proc.apvts.state.getProperty (kSizeKey, kDefaultWidth));
    setResizable (true, true);
    setResizeLimits (688, 384, 2064, 1152);
    getConstrainer()->setFixedAspectRatio ((double) kW / (double) kH);
    setSize (w, juce::roundToInt ((float) w * kH / kW));
    sizeReady = true;
    resized();
    startTimerHz (30);
}

MeteoraTapeEditor::~MeteoraTapeEditor()
{
    for (auto& s : knobs) s.setLookAndFeel (nullptr);
}

void MeteoraTapeEditor::resized()
{
    if (getWidth() <= 0 || getHeight() <= 0) return;
    if (sizeReady) proc.apvts.state.setProperty (kSizeKey, getWidth(), nullptr);
    bgK = juce::jmax (1.0f, juce::Component::getApproximateScaleFactorForComponent (this));
    bgScaled = bg.rescaled (juce::roundToInt ((float) getWidth() * bgK), juce::roundToInt ((float) getHeight() * bgK),
                            juce::Graphics::highResamplingQuality);
    for (auto& r : reels) prepareReel (r);
    const float s = (float) getWidth() / kW;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const auto& k = kKnobs[i];
        knobs[i].setBounds (juce::Rectangle<float> ((k.cx - k.r) * s, (k.cy - k.r) * s, 2.0f * k.r * s, 2.0f * k.r * s).toNearestInt());
    }
    for (auto& r : reels) renderReel (r);
}

// texture e luce della bobina alla grandezza della finestra (una volta sola, quando cambia la grandezza)
void MeteoraTapeEditor::prepareReel (Reel& r)
{
    const float px = (float) getWidth() / kW * bgK;               // pixel per unita' dell'immagine
    const int n = juce::jmax (8, juce::roundToInt (2.0f * r.a * px));
    r.tex = r.face.rescaled (n, n, juce::Graphics::highResamplingQuality).convertedToFormat (juce::Image::RGB);
    r.box = juce::Rectangle<float> ((r.cx - r.a) * px, (r.cy - r.b) * px, 2.0f * r.a * px, 2.0f * r.b * px).getSmallestIntegerContainer();
    r.lightScaled = r.light.rescaled (juce::jmax (1, r.box.getWidth()), juce::jmax (1, r.box.getHeight()),
                                      juce::Graphics::highResamplingQuality).convertedToFormat (juce::Image::RGB);
    r.frame = juce::Image (juce::Image::ARGB, juce::jmax (1, r.box.getWidth()), juce::jmax (1, r.box.getHeight()), true);
}

// disegna la bobina girata: la texture ruota nel suo cerchio, poi si schiaccia in prospettiva e prende la luce della foto
void MeteoraTapeEditor::renderReel (Reel& r)
{
    if (! r.tex.isValid() || ! r.frame.isValid()) return;
    const float px = (float) getWidth() / kW * bgK;
    const float A = r.a * px, B = r.b * px, cxp = r.cx * px - (float) r.box.getX(), cyp = r.cy * px - (float) r.box.getY();
    const int n = r.tex.getWidth();
    const float half = (float) n * 0.5f, ratio = A / B;
    const float c = std::cos (reelAngle), s = std::sin (reelAngle);
    juce::Image::BitmapData src (r.tex, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData lit (r.lightScaled, juce::Image::BitmapData::readOnly);
    juce::Image::BitmapData dst (r.frame, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < dst.height; ++y)
    {
        const float dy = ((float) y + 0.5f - cyp) * ratio;
        for (int x = 0; x < dst.width; ++x)
        {
            const float dx = (float) x + 0.5f - cxp;
            const float rr = std::sqrt (dx * dx + dy * dy);
            auto* out = dst.getPixelPointer (x, y);
            const float edge = juce::jlimit (0.0f, 1.0f, (A - rr) * 0.9f + 0.5f);      // bordo morbido dell'ellisse
            if (edge <= 0.0f) { out[0] = out[1] = out[2] = out[3] = 0; continue; }
            // punto della texture: rotazione inversa
            const float u = c * dx + s * dy + half, v = -s * dx + c * dy + half;
            const int iu = juce::jlimit (0, n - 2, (int) u), iv = juce::jlimit (0, n - 2, (int) v);
            const float fu = juce::jlimit (0.0f, 1.0f, u - (float) iu), fv = juce::jlimit (0.0f, 1.0f, v - (float) iv);
            const auto* p00 = src.getPixelPointer (iu, iv);
            const auto* p10 = src.getPixelPointer (iu + 1, iv);
            const auto* p01 = src.getPixelPointer (iu, iv + 1);
            const auto* p11 = src.getPixelPointer (iu + 1, iv + 1);
            const auto* lp = lit.getPixelPointer (juce::jmin (x, lit.width - 1), juce::jmin (y, lit.height - 1));
            const float g = (float) lp[1] / 127.5f;                                     // luce della foto (0..2)
            float col[3];
            for (int k = 0; k < 3; ++k)
            {
                const float top = (float) p00[k] + ((float) p10[k] - (float) p00[k]) * fu;
                const float bot = (float) p01[k] + ((float) p11[k] - (float) p01[k]) * fu;
                col[k] = juce::jlimit (0.0f, 255.0f, (top + (bot - top) * fv) * g);
            }
            // ARGB premoltiplicato
            const float a = edge * 255.0f;
            auto* pix = reinterpret_cast<juce::PixelARGB*> (out);
            pix->setARGB ((juce::uint8) a, (juce::uint8) (col[2] * edge), (juce::uint8) (col[1] * edge), (juce::uint8) (col[0] * edge));
        }
    }
}

void MeteoraTapeEditor::setParam (const char* id, float plain)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
        prm->endChangeGesture();
    }
}

void MeteoraTapeEditor::linkOutput (float newIn)
{
    auto* o = proc.apvts.getParameter ("out");
    if (o == nullptr) { prevIn = newIn; return; }
    if (linkDrag)
    {
        const float nv = juce::jlimit (-24.0f, 12.0f, linkOut0 - (newIn - linkIn0));
        o->setValueNotifyingHost (o->convertTo0to1 (nv));
    }
    else if (knobs[0].isMouseOver())
    {
        const float nv = juce::jlimit (-24.0f, 12.0f, P ("out") - (newIn - prevIn));
        o->beginChangeGesture();
        o->setValueNotifyingHost (o->convertTo0to1 (nv));
        o->endChangeGesture();
    }
    prevIn = newIn;
}

juce::String MeteoraTapeEditor::valueText (int i) const
{
    const double v = knobs[(size_t) i].getValue();
    if (i == 2)
    {
        if (std::abs (v) < 0.5) return "Normal";
        return (v < 0.0 ? "Under " : "Over ") + juce::String ((int) std::lround (std::abs (v))) + " %";
    }
    return (v >= 0.0 ? "+" : "") + juce::String (v, 1) + " dB";
}

void MeteoraTapeEditor::paint (juce::Graphics& g)
{
    const float s = (float) getWidth() / kW;
    auto sc = [s] (float v) { return v * s; };
    const auto base = juce::AffineTransform::scale (1.0f / bgK);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImageTransformed (bgScaled, base);

    // bobine che girano e, sopra, le parti ferme (mozzi, coperchio delle testine, perni)
    for (auto& r : reels)
    {
        g.drawImage (r.frame, juce::Rectangle<float> ((float) r.box.getX() / bgK, (float) r.box.getY() / bgK,
                                                      (float) r.box.getWidth() / bgK, (float) r.box.getHeight() / bgK));
        g.drawImage (r.fg, juce::Rectangle<float> (sc (r.fx), sc (r.fy), sc (r.fw), sc (r.fh)));
    }

    // VU dei canali in alto e loro lampadine: accesi solo quando il plugin lavora
    for (int i = 0; i < 8; ++i)
    {
        const float lx = 207.5f + 125.0f * (float) i;
        if (glow < 0.999f)
        {
            g.setColour (juce::Colour (0xff0c0a08).withAlpha (0.62f * (1.0f - glow)));
            g.fillRect (juce::Rectangle<float> (sc (lx - 13.0f), 0.0f, sc (106.0f), sc (47.0f)));
            g.setColour (juce::Colour (0xff15110c).withAlpha (0.8f * (1.0f - glow)));
            g.fillRect (juce::Rectangle<float> (sc (lx - 6.5f), sc (kLampY - 6.5f), sc (13.0f), sc (13.0f)));
        }
        if (glow > 0.001f)
        {
            juce::ColourGradient cg (juce::Colour (0xffffb347).withAlpha (0.45f * glow), sc (lx), sc (kLampY),
                                     juce::Colour (0xffffb347).withAlpha (0.0f), sc (lx + 16.0f), sc (kLampY), true);
            g.setGradientFill (cg);
            g.fillEllipse (sc (lx - 16.0f), sc (kLampY - 16.0f), sc (32.0f), sc (32.0f));
        }
    }

    // SATURATION: puntino chiaro sulla gonna del pomello
    {
        const double prop = knobs[0].valueToProportionOfLength (knobs[0].getValue());
        const float a = juce::degreesToRadians (kSatA0 + (kSatA1 - kSatA0) * (float) prop);
        const float x = sc (kSatX + std::sin (a) * kSatR), y = sc (kSatY - std::cos (a) * kSatR);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (x - sc (3.6f), y - sc (3.2f), sc (7.4f), sc (7.4f));
        g.setColour (juce::Colour (0xffece4d2));
        g.fillEllipse (x - sc (2.7f), y - sc (2.7f), sc (5.4f), sc (5.4f));
    }
    // MAKEUP GAIN e BIAS: riga scura incisa sulla faccia del pomello
    auto engraved = [&] (float cx, float cy, float R, float deg, float r0, float r1, float width)
    {
        const float a = juce::degreesToRadians (deg);
        auto pt = [&] (float rr) { return juce::Point<float> (sc (cx + std::sin (a) * rr), sc (cy - std::cos (a) * rr)); };
        const juce::Point<float> hl (sc (0.5f), sc (0.8f));
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.drawLine (juce::Line<float> (pt (r0 * R) + hl, pt (r1 * R) + hl), sc (width));
        g.setColour (juce::Colour (0xff141210));
        g.drawLine (juce::Line<float> (pt (r0 * R), pt (r1 * R)), sc (width));
    };
    {
        const double prop = knobs[1].valueToProportionOfLength (knobs[1].getValue());
        engraved (kOutX, kOutY, kOutR, kOutA0 + (kOutA1 - kOutA0) * (float) prop, 0.64f, 0.96f, 3.0f);
        engraved (kBiasX, kBiasY, kBiasR, -2.5f + 34.5f * (float) (knobs[2].getValue() / 100.0), 0.25f, 0.78f, 2.4f);
    }

    // TAPE SPEED: il pomello a becco ruota sullo scatto scelto
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path c;
        c.addEllipse (sc (kSelX - kSelR), sc (kSelY - kSelR), sc (2.0f * kSelR), sc (2.0f * kSelR));
        g.reduceClipRegion (c);
        g.drawImageTransformed (bgScaled, base.rotated (juce::degreesToRadians (ipsAngle), sc (kSelX), sc (kSelY)));
    }

    // LO-FI: levetta giu' (spento) o su (acceso)
    g.drawImage (P ("lofi") > 0.5f ? lofiOn : lofiOff, juce::Rectangle<float> (sc (kLofiX), sc (kLofiY), sc (kLofiW), sc (kLofiH)));

    // VU: lancette vere, quadranti spenti quando il plugin non lavora, OUTPUT che arrossisce quando il nastro satura
    for (int m = 0; m < 2; ++m)
    {
        const float px = kVuX[m], py = kVuY[m], fy = py - 47.0f;
        const float vu = m == 0 ? inNeedle : outNeedle;
        const float p = juce::jlimit (0.0f, 1.12f, std::pow (10.0f, vu / 20.0f) / 1.4125f);
        const float a = juce::degreesToRadians (-46.0f + 92.0f * (p - 0.0708f) / 0.9292f);
        auto pt = [&] (float rr) { return juce::Point<float> (sc (px + std::sin (a) * rr), sc (py - std::cos (a) * rr)); };
        const juce::Point<float> off (sc (1.4f), sc (1.6f));
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.drawLine (juce::Line<float> (pt (30.0f) + off, pt (86.0f) + off), sc (1.6f));
        g.setColour (juce::Colour (0xff1c140d));
        g.drawLine (juce::Line<float> (pt (30.0f), pt (86.0f)), sc (1.6f));
        juce::Path face;
        face.addEllipse (sc (px - 86.0f), sc (fy - 86.0f), sc (172.0f), sc (172.0f));
        if (glow < 0.999f)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (face);
            g.setColour (juce::Colour (0xff0e0b08).withAlpha (0.66f * (1.0f - glow)));
            g.fillRect (getLocalBounds());
        }
        if (m == 1 && satRed * glow > 0.003f)
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (face);
            const float k = satRed * glow;
            juce::ColourGradient cg (juce::Colour (0xffff3a1c).withAlpha (0.55f * k), sc (px), sc (fy - 8.0f),
                                     juce::Colour (0xffb01008).withAlpha (0.35f * k), sc (px + 86.0f), sc (fy - 8.0f), true);
            g.setGradientFill (cg);
            g.fillRect (getLocalBounds());
        }
    }

    // logo Meteora: si accende come il filamento di una valvola, luce morbida
    if (tube > 0.003f)
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.setOpacity (juce::jlimit (0.0f, 1.0f, 0.55f * std::pow (tube, 1.4f) * flick));
        g.drawImage (glowImg, juce::Rectangle<float> (sc (kLogoX), sc (kLogoY), sc (kLogoS), sc (kLogoS)));
    }

    // valore del comando che si sta toccando
    if (lastKnob >= 0 || showIps > 0 || showLofi > 0)
    {
        auto d = juce::Rectangle<float> (sc (538.0f), sc (394.0f), sc (300.0f), sc (30.0f));
        g.setColour (juce::Colour (0xcc14130f));
        g.fillRoundedRectangle (d, sc (6.0f));
        g.setColour (juce::Colour (0xffe8c49a));
        g.setFont (sc (17.0f));
        juce::String txt;
        if (showIps > 0)       txt = juce::String ("Tape Speed   ") + (lastIps == 0 ? "7.5" : lastIps == 1 ? "15" : "30") + " IPS";
        else if (showLofi > 0) txt = juce::String ("Lo-Fi   ") + (P ("lofi") > 0.5f ? "On" : "Off");
        else                   txt = juce::String (kKnobs[lastKnob].name) + "   " + valueText (lastKnob);
        g.drawText (txt, d.toNearestInt(), juce::Justification::centred);
    }
}

void MeteoraTapeEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        const std::array<int, 6> pcts { 50, 67, 75, 100, 125, 150 };
        juce::PopupMenu m;
        for (int i = 0; i < (int) pcts.size(); ++i)
        {
            const int w = juce::roundToInt (kW * (float) pcts[(size_t) i] / 100.0f);
            m.addItem (i + 1, "Size " + juce::String (pcts[(size_t) i]) + " %", true, std::abs (getWidth() - w) < 4);
        }
        juce::Component::SafePointer<MeteoraTapeEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withMousePosition(), [safe, pcts] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            const int w = juce::roundToInt (kW * (float) pcts[(size_t) (r - 1)] / 100.0f);
            safe->setSize (w, juce::roundToInt ((float) w * kH / kW));
        });
        return;
    }
    const auto p = e.position / ((float) getWidth() / kW);
    // TAPE SPEED: la velocita' piu' vicina al punto cliccato (sul pomello: la successiva)
    if (juce::Rectangle<float> (1080.0f, 440.0f, 180.0f, 165.0f).contains (p))
    {
        const float dx = p.x - kSelX, dy = p.y - kSelY;
        int v;
        if (std::hypot (dx, dy) < 34.0f) v = (juce::jlimit (0, 2, (int) std::lround (P ("ips"))) + 1) % 3;
        else
        {
            const float ang = juce::radiansToDegrees (std::atan2 (dx, -dy));
            v = ang < -23.0f ? 0 : (ang > 23.0f ? 2 : 1);
        }
        setParam ("ips", (float) v);
        showIps = 60; showLofi = 0; lastKnob = -1;
        repaint();
        return;
    }
    // LO-FI
    if (juce::Rectangle<float> (64.0f, 72.0f).withCentre ({ 1240.0f, 682.0f }).contains (p))
    {
        setParam ("lofi", P ("lofi") > 0.5f ? 0.0f : 1.0f);
        showLofi = 60; showIps = 0; lastKnob = -1;
        repaint();
    }
}

void MeteoraTapeEditor::timerCallback()
{
    const bool running = proc.isRunning();
    const int ips = juce::jlimit (0, 2, (int) std::lround (P ("ips")));
    bool dirty = false;
    if (ips != lastIps) { lastIps = ips; showIps = 60; dirty = true; }
    if (showIps > 0 && --showIps == 0) dirty = true;
    if (showLofi > 0 && --showLofi == 0) dirty = true;

    const float tgtA = kSelAngle[ips];
    if (std::abs (tgtA - ipsAngle) > 0.05f) { ipsAngle += (tgtA - ipsAngle) * 0.35f; dirty = true; }

    // luci: VU e lampadine morbide, logo-valvola che si scalda piano e tremola appena
    const float gPrev = glow, tPrev = tube, fPrev = flick;
    glow += ((running ? 1.0f : 0.0f) - glow) * 0.12f;
    if (running) tube += (1.0f - tube) * 0.045f; else tube *= 0.88f;
    if (tube < 0.002f) tube = 0.0f;
    flick = running ? 0.96f + 0.04f * rnd.nextFloat() : 1.0f;
    if (std::abs (glow - gPrev) > 0.003f || std::abs (tube - tPrev) > 0.002f || std::abs (flick - fPrev) > 0.004f) dirty = true;

    // rosso del VU OUTPUT: da +6 VU sul nastro fino a pieno a +14 VU
    {
        const float sTarget = running ? juce::jlimit (0.0f, 1.0f, (proc.inVu.load() - 6.0f) / 8.0f) : 0.0f;
        const float sPrev = satRed;
        satRed += (sTarget - satRed) * (sTarget > satRed ? 0.5f : 0.08f);
        if (std::abs (satRed - sPrev) > 0.003f) dirty = true;
    }
    // lancette con la balistica dei VU (~300 ms); a riposo tornano a sinistra
    const float iT = running ? proc.inVu.load() : -40.0f, oT = running ? proc.outVu.load() : -40.0f;
    const float iPrev = inNeedle, oPrev = outNeedle;
    inNeedle += (iT - inNeedle) * 0.22f;
    outNeedle += (oT - outNeedle) * 0.22f;
    if (std::abs (inNeedle - iPrev) > 0.02f || std::abs (outNeedle - oPrev) > 0.02f) dirty = true;

    // bobine: girano quando il plugin lavora, piu' veloci a 30 IPS; partono e si fermano con un po' d'inerzia
    const float revPerSec = running ? (ips == 0 ? 0.3f : ips == 1 ? 0.6f : 1.2f) : 0.0f;
    reelSpeed += (revPerSec - reelSpeed) * (running ? 0.08f : 0.05f);
    bool reelsMoved = false;
    if (reelSpeed > 0.002f)
    {
        reelAngle -= reelSpeed * juce::MathConstants<float>::twoPi / 30.0f;      // senso antiorario, come in riproduzione
        if (reelAngle < -juce::MathConstants<float>::twoPi) reelAngle += juce::MathConstants<float>::twoPi;
        for (auto& r : reels) renderReel (r);
        reelsMoved = true;
    }
    if (dirty) repaint();
    else if (reelsMoved)
    {
        const float sl = (float) getWidth() / kW;
        repaint (juce::Rectangle<float> (146.0f * sl, 95.0f * sl, 1090.0f * sl, 230.0f * sl).getSmallestIntegerContainer());
    }
}
