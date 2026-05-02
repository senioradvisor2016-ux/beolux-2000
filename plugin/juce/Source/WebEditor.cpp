/*  BC2000DLWebEditor implementation. */

#include "WebEditor.h"
#include "BinaryData.h"

namespace
{
    // 30 % nedskalning av Ferroflux-chassi (1400 × ~1000 → 980 × 700).
    // CSS transform: scale(.7) i ferroflux.html (body.in-juce) renderar tight.
    // 90 % CSS-skalning av 1400-px Ferroflux-chassi → editor 1280 × 920.
    // Var 70 %/990 × 700 — texten på knapparna blev oläslig.
    constexpr int kEditorW = 1280;
    constexpr int kEditorH = 920;

    // BinaryData för embeddade web-assets (genererat av juce_add_binary_data
    // med target BC2000DL_WebAssets — exporterar BC2000DL_WebAssetsData::*).
    // Filnamnet "ferroflux.html" mappas till symbolen "ferroflux_html".
    std::optional<juce::WebBrowserComponent::Resource>
    fetchEmbeddedResource (const juce::String& path)
    {
        // path börjar med "/" — JUCE serverar virtuellt root-relativt
        juce::String key = path;
        if (key.startsWith ("/")) key = key.substring (1);
        if (key.isEmpty()) key = "ferroflux.html";

        int sizeBytes = 0;
        const char* data = nullptr;
        const char* mime = "application/octet-stream";

        // Pröva känt mappning först
        if (key == "ferroflux.html" || key == "index.html")
        {
            data = BinaryData::getNamedResource ("ferroflux_html", sizeBytes);
            mime = "text/html; charset=utf-8";
        }
        else
        {
            // Generic lookup
            const auto symbolName = key.replace ("/", "_").replace (".", "_").replace ("-", "_");
            data = BinaryData::getNamedResource (symbolName.toRawUTF8(), sizeBytes);

            if (key.endsWithIgnoreCase (".css"))  mime = "text/css; charset=utf-8";
            else if (key.endsWithIgnoreCase (".js")) mime = "application/javascript; charset=utf-8";
            else if (key.endsWithIgnoreCase (".html")) mime = "text/html; charset=utf-8";
            else if (key.endsWithIgnoreCase (".svg")) mime = "image/svg+xml";
            else if (key.endsWithIgnoreCase (".png")) mime = "image/png";
            else if (key.endsWithIgnoreCase (".jpg") || key.endsWithIgnoreCase (".jpeg"))
                mime = "image/jpeg";
            else if (key.endsWithIgnoreCase (".woff2")) mime = "font/woff2";
            else if (key.endsWithIgnoreCase (".woff"))  mime = "font/woff";
        }

        if (data == nullptr || sizeBytes <= 0)
            return std::nullopt;

        juce::WebBrowserComponent::Resource res;
        res.data = std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + sizeBytes);
        res.mimeType = mime;
        return res;
    }
}

BC2000DLWebEditor::BC2000DLWebEditor (BC2000DLProcessor& p)
    : juce::AudioProcessorEditor (&p),
      processor (p),
      webView (juce::WebBrowserComponent::Options{}
        .withNativeIntegrationEnabled (true)
        .withResourceProvider ([] (const auto& path) { return fetchEmbeddedResource (path); })
        .withEventListener ("paramChange",
            [this] (const juce::var& v) { onParamChangeFromJS (v); }))
{
    setSize (kEditorW, kEditorH);
    setOpaque (true);
    addAndMakeVisible (webView);

    // Ladda embeddad index — resourceProvider serverar från virtuellt root
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot()
                      + "ferroflux.html");

    startTimerHz (30);
}

void BC2000DLWebEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void BC2000DLWebEditor::onParamChangeFromJS (const juce::var& payload)
{
    if (! payload.isObject()) return;

    const auto idVar    = payload.getProperty ("id", juce::var{});
    const auto valueVar = payload.getProperty ("value", juce::var{});
    if (! idVar.isString()) return;

    const auto paramId = idVar.toString();
    auto& apvts = processor.apvts;

    auto setParamFromValue = [&] (const juce::String& id, double rawValue)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (static_cast<float> (rawValue)));
    };

    const double v = static_cast<double> (valueVar);
    setParamFromValue (paramId, v);

    // Stereo-länkning för saturation: Ferroflux har bara en SAT-knob, men DSP:n
    // har separat L/R. Spegla automatiskt så båda kanaler får samma drive.
    if (paramId == "saturation_drive")
        setParamFromValue ("saturation_drive_r", v);
}

void BC2000DLWebEditor::timerCallback()
{
    auto& chain = processor.getChain();
    const float lvlL = chain.meterLevelL_dBFS.load();
    const float lvlR = chain.meterLevelR_dBFS.load();

    // Peak-hold med 0.18 dB/frame fall-off (matchar Ferroflux-app:n)
    if (lvlL > peakL) peakL = lvlL; else peakL -= 0.18f;
    if (lvlR > peakR) peakR = lvlR; else peakR -= 0.18f;
    peakL = juce::jmax (-30.0f, peakL);
    peakR = juce::jmax (-30.0f, peakR);

    juce::DynamicObject::Ptr obj = new juce::DynamicObject();
    obj->setProperty ("L",      lvlL);
    obj->setProperty ("R",      lvlR);
    obj->setProperty ("peakL",  peakL);
    obj->setProperty ("peakR",  peakR);
    obj->setProperty ("recL",   chain.isRecordingL.load());
    obj->setProperty ("recR",   chain.isRecordingR.load());

    webView.emitEventIfBrowserIsVisible ("meters", juce::var (obj.get()));
}
