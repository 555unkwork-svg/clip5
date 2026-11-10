#include "Presets.h"
#include "Params.h"

namespace clip5
{
using namespace params;

namespace
{
    struct Val { const char* id; float v; };
    struct Factory { const char* name; std::vector<Val> vals; };

    // modo: 0 Suave, 1 Firme, 2 Duro, 3 Apagado · tono: 0 Plano, 1 Seda, 2 Terciopelo
    // En los presets paralelos con techo bajo el Techo estricto va apagado: está al final de la
    // cadena y limitaría también la señal SECA al nivel del techo.
    const std::vector<Factory>& factory()
    {
        static const std::vector<Factory> f = {
            { "Transparente", { { id::entrada, 0 }, { id::densidad, 0 }, { id::techo, -1 }, { id::modo, 0 },
                                { id::suavizado, 0 }, { id::tono, 0 } } },
            { "Mastering pop", { { id::entrada, 3 }, { id::densidad, 2 }, { id::techo, -1 }, { id::modo, 1 },
                                 { id::suavizado, 35 }, { id::tono, 0 } } },
            { "Densidad sin recorte", { { id::entrada, 0 }, { id::densidad, 4 }, { id::techo, -0.3f }, { id::modo, 3 },
                                        { id::suavizado, 20 }, { id::gananciaUnitaria, 1 } } },
            { "Bus de batería", { { id::entrada, 6 }, { id::densidad, 3 }, { id::techo, -6 }, { id::modo, 2 },
                                  { id::suavizado, 20 }, { id::humedo, 0 }, { id::seco, -6 }, { id::enlaceMezcla, 0 },
                                  { id::techoEstricto, 0 } } },
            { "Rock denso", { { id::entrada, 5 }, { id::densidad, 4 }, { id::techo, -1 }, { id::modo, 1 },
                              { id::suavizado, 50 }, { id::tono, 1 } } },
            { "Electrónica al límite", { { id::entrada, 8 }, { id::densidad, 5 }, { id::techo, -0.5f }, { id::modo, 2 },
                                         { id::suavizado, 30 }, { id::tono, 0 } } },
            { "Voz al frente", { { id::entrada, 4 }, { id::densidad, 3 }, { id::techo, -3 }, { id::modo, 0 },
                                 { id::suavizado, 60 }, { id::tono, 2 }, { id::seco, -10 }, { id::enlaceMezcla, 0 },
                                 { id::techoEstricto, 0 } } },
            { "Paralelo sucio", { { id::entrada, 10 }, { id::densidad, 6 }, { id::techo, -12 }, { id::modo, 2 }, { id::salida, -3 },
                                  { id::suavizado, 0 }, { id::tono, 2 }, { id::humedo, -6 }, { id::seco, 0 },
                                  { id::enlaceMezcla, 0 }, { id::techoEstricto, 0 } } },
        };
        return f;
    }

    constexpr auto kExt = ".clip5preset";
}

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s), current (es ("Transparente")) {}

bool PresetManager::isExcludedFromPresets (const juce::String& pid)
{
    return pid == id::calidad || pid == id::calidadOffline || pid == id::fase;
}

int PresetManager::numFactory() const noexcept { return (int) factory().size(); }
juce::String PresetManager::factoryName (int i) const { return es (factory()[(size_t) i].name); }

void PresetManager::setParam (const char* pid, float value)
{
    if (auto* p = apvts.getParameter (pid))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

void PresetManager::resetToDefaults()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! isExcludedFromPresets (rp->paramID))
            {
                rp->beginChangeGesture();
                rp->setValueNotifyingHost (rp->getDefaultValue());
                rp->endChangeGesture();
            }
}

void PresetManager::loadFactory (int i)
{
    if (i < 0 || i >= numFactory()) return;
    resetToDefaults();
    for (const auto& v : factory()[(size_t) i].vals) setParam (v.id, v.v);
    current = factoryName (i);
}

juce::File PresetManager::userFolder() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("clip5").getChildFile ("Presets");
}

juce::Array<juce::File> PresetManager::userPresets() const
{
    auto files = userFolder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + kExt);
    files.sort();
    return files;
}

bool PresetManager::saveUser (const juce::String& name)
{
    auto folder = userFolder();
    if (! folder.createDirectory()) return false;
    juce::XmlElement xml ("CLIP5PRESET");
    xml.setAttribute ("version", kStateVersion);
    xml.setAttribute ("name", name);
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! isExcludedFromPresets (rp->paramID))
            {
                auto* e = xml.createNewChildElement ("PARAM");
                e->setAttribute ("id", rp->paramID);
                e->setAttribute ("value", rp->convertFrom0to1 (rp->getValue()));
            }
    const auto file = folder.getChildFile (juce::File::createLegalFileName (name) + kExt);
    if (! xml.writeTo (file)) return false;
    current = name;
    return true;
}

bool PresetManager::loadUser (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr || ! xml->hasTagName ("CLIP5PRESET")) return false;
    resetToDefaults();
    for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
    {
        const auto pidStr = e->getStringAttribute ("id");
        if (isExcludedFromPresets (pidStr)) continue;
        setParam (pidStr.toRawUTF8(), (float) e->getDoubleAttribute ("value"));
    }
    current = xml->getStringAttribute ("name", f.getFileNameWithoutExtension());
    return true;
}

int PresetManager::count() const { return numFactory() + userPresets().size(); }

juce::String PresetManager::nameAt (int i) const
{
    if (i < numFactory()) return factoryName (i);
    const auto u = userPresets();
    const int k = i - numFactory();
    return k < u.size() ? u[k].getFileNameWithoutExtension() : juce::String();
}

void PresetManager::loadAt (int i)
{
    if (i < numFactory()) { loadFactory (i); return; }
    const auto u = userPresets();
    const int k = i - numFactory();
    if (k < u.size()) loadUser (u[k]);
}

int PresetManager::indexOfCurrent() const
{
    for (int i = 0; i < count(); ++i)
        if (nameAt (i) == current) return i;
    return -1;
}
} // namespace clip5
