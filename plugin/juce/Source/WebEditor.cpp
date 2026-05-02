/*  BC2000DLWebEditor implementation. */

#include "WebEditor.h"
#include "BinaryData.h"

namespace
{
    // BEOFLUX 2000DL TEAC-design: 1280×780 (passar MacBook Air 1440×900).
    constexpr int kEditorW = 1280;
    constexpr int kEditorH = 780;

    // BinaryData för embeddade web-assets (genererat av juce_add_binary_data
    // med target BC2000DL_WebAssets — exporterar BC2000DL_WebAssetsData::*).
    // Filnamnet "ferroflux.html" mappas till symbolen "ferroflux_html".
    std::optional<juce::WebBrowserComponent::Resource>
    fetchEmbeddedResource (const juce::String& path)
    {
        // path börjar med "/" — JUCE serverar virtuellt root-relativt
        juce::String key = path;
        if (key.startsWith ("/")) key = key.substring (1);
        if (key.isEmpty()) key = "beoflux.html";

        int sizeBytes = 0;
        const char* data = nullptr;
        const char* mime = "application/octet-stream";

        // Pröva känt mappning först
        if (key == "beoflux.html" || key == "index.html")
        {
            data = BinaryData::getNamedResource ("beoflux_html", sizeBytes);
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
            [this] (const juce::var& v) { onParamChangeFromJS (v); })
        .withEventListener ("requestSync",
            [this] (const juce::var&) { pushFullStateToJS(); })
        .withEventListener ("requestPresets",
            [this] (const juce::var&) { pushPresetListToJS(); })
        .withEventListener ("presetChange",
            [this] (const juce::var& v) { onPresetChangeFromJS (v); }))
{
    setSize (kEditorW, kEditorH);
    setOpaque (true);
    setResizable (true, true);
    setResizeLimits (960, 600, 1920, 1200);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio ((double) kEditorW / (double) kEditorH);
    addAndMakeVisible (webView);

    // Ladda embeddad index — resourceProvider serverar från virtuellt root
    webView.goToURL (juce::WebBrowserComponent::getResourceProviderRoot()
                      + "beoflux.html");

    // Live host-automation -> UI sync
    static const std::vector<juce::String> kListenedParams = {
        "speed", "tape_formula",
        "mic_gain", "mic_gain_r",
        "phono_gain", "phono_gain_r",
        "radio_gain", "radio_gain_r",
        "saturation_drive", "saturation_drive_r",
        "echo_amount", "echo_amount_r",
        "treble_db", "bass_db", "balance",
        "bias_amount", "wow_flutter",
        "echo_enabled", "bypass_tape", "synchroplay",
        "multiplay_gen",
        "rec_arm_1", "rec_arm_2", "track_1", "track_2",
        "monitor_mode", "pause",
        "speaker_ext", "speaker_int", "speaker_mute",
        "sos_enabled", "pa_enabled",
        "mic_loz", "phono_mode", "radio_mode"
    };
    for (const auto& id : kListenedParams)
        processor.apvts.addParameterListener (id, this);

    startTimerHz (30);
}

BC2000DLWebEditor::~BC2000DLWebEditor()
{
    static const std::vector<juce::String> kListenedParams = {
        "speed", "tape_formula",
        "mic_gain", "mic_gain_r",
        "phono_gain", "phono_gain_r",
        "radio_gain", "radio_gain_r",
        "saturation_drive", "saturation_drive_r",
        "echo_amount", "echo_amount_r",
        "treble_db", "bass_db", "balance",
        "bias_amount", "wow_flutter",
        "echo_enabled", "bypass_tape", "synchroplay",
        "multiplay_gen",
        "rec_arm_1", "rec_arm_2", "track_1", "track_2",
        "monitor_mode", "pause",
        "speaker_ext", "speaker_int", "speaker_mute",
        "sos_enabled", "pa_enabled",
        "mic_loz", "phono_mode", "radio_mode"
    };
    for (const auto& id : kListenedParams)
        processor.apvts.removeParameterListener (id, this);
}

void BC2000DLWebEditor::parameterChanged (const juce::String& parameterID, float /*newValue*/)
{
    if (auto* prm = processor.apvts.getParameter (parameterID))
    {
        const float v01 = prm->getValue();
        // Echo suppression: if this matches the last value JS sent for this
        // param, the change came from our own UI — don't push back.
        auto it = lastJSValue.find (parameterID);
        if (it != lastJSValue.end() && std::abs (it->second - v01) < 1e-5f)
            return;
        juce::MessageManager::callAsync ([this, parameterID, v01]() {
            pushOneParamToJS (parameterID, v01);
        });
    }
}

void BC2000DLWebEditor::pushOneParamToJS (const juce::String& id, float value01)
{
    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty ("id",    id);
    o->setProperty ("value", (double) value01);
    webView.emitEventIfBrowserIsVisible ("paramSync", juce::var (o.get()));
}

void BC2000DLWebEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

void BC2000DLWebEditor::onParamChangeFromJS (const juce::var& payload)
{
    // BEOFLUX bridgen skickar redan normaliserat 0..1 — sätt direkt.
    if (! payload.isObject()) return;
    const auto idVar    = payload.getProperty ("id", juce::var{});
    const auto valueVar = payload.getProperty ("value", juce::var{});
    if (! idVar.isString()) return;

    const auto paramId = idVar.toString();
    const auto value01 = juce::jlimit (0.0f, 1.0f,
                                       static_cast<float> ((double) valueVar));
    // Track this so the listener echo-back is suppressed for OUR own UI-originated changes
    lastJSValue[paramId] = value01;
    if (auto* prm = processor.apvts.getParameter (paramId))
        prm->setValueNotifyingHost (value01);
}

void BC2000DLWebEditor::pushPresetListToJS()
{
    juce::Array<juce::var> names;
    const int n = processor.getNumPrograms();
    for (int i = 0; i < n; ++i)
        names.add (processor.getProgramName (i));

    juce::DynamicObject::Ptr o = new juce::DynamicObject();
    o->setProperty ("names", juce::var (names));
    o->setProperty ("index", processor.getCurrentProgram());
    webView.emitEventIfBrowserIsVisible ("presetSync", juce::var (o.get()));
}

void BC2000DLWebEditor::onPresetChangeFromJS (const juce::var& payload)
{
    if (! payload.isObject()) return;
    const int idx = (int) payload.getProperty ("index", juce::var (0));
    processor.setCurrentProgram (idx);
    // Push synced parameter values back so HTML reflects new preset
    pushFullStateToJS();
}

void BC2000DLWebEditor::pushFullStateToJS()
{
    // Skicka initial 0..1 för varje parameter så UI:t synkar med APVTS-state
    // (t.ex. vid preset-load eller när host återställer state).
    auto& apvts = processor.apvts;
    static constexpr const char* ids[] = {
        "speed", "tape_formula",
        "mic_gain", "mic_gain_r",
        "phono_gain", "phono_gain_r",
        "radio_gain", "radio_gain_r",
        "saturation_drive", "saturation_drive_r",
        "echo_amount", "echo_amount_r",
        "treble_db", "bass_db", "balance",
        "bias_amount", "wow_flutter",
        "echo_enabled", "bypass_tape", "synchroplay",
        "multiplay_gen",
        "rec_arm_1", "rec_arm_2", "track_1", "track_2",
        "monitor_mode", "pause",
        "speaker_ext", "speaker_int", "speaker_mute",
        "sos_enabled", "pa_enabled",
        "mic_loz", "phono_mode", "radio_mode"
    };
    for (auto* id : ids)
    {
        if (auto* prm = apvts.getParameter (juce::String (id)))
        {
            juce::DynamicObject::Ptr o = new juce::DynamicObject();
            o->setProperty ("id",    juce::String (id));
            o->setProperty ("value", (double) prm->getValue());
            webView.emitEventIfBrowserIsVisible ("paramSync", juce::var (o.get()));
        }
    }
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
