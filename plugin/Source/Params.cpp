#include "Params.h"

namespace clip5::params
{
juce::String formatDb (float db, int decimals)
{
    auto s = juce::String (std::abs (db), decimals).replaceCharacter ('.', ',');
    if (db > 0.0f && s != juce::String (0.0, decimals).replaceCharacter ('.', ',')) return "+" + s;
    if (db < 0.0f && s != juce::String (0.0, decimals).replaceCharacter ('.', ',')) return es ("−") + s;
    return s;
}

juce::String formatFader (float db)
{
    return db <= kFaderMinDb + 0.05f ? es ("−∞") : formatDb (db);
}

namespace
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    using APC = juce::AudioParameterChoice;
    using Attr = juce::AudioParameterFloatAttributes;

    juce::ParameterID pid (const char* s) { return { s, kStateVersion }; }

    std::unique_ptr<APF> dbParam (const char* id, const char* name, float lo, float hi, float def, float step = 0.01f)
    {
        return std::make_unique<APF> (pid (id), es (name), juce::NormalisableRange<float> (lo, hi, step), def,
                                      Attr().withLabel ("dB")
                                            .withStringFromValueFunction ([] (float v, int) { return formatDb (v) + " dB"; })
                                            .withValueFromStringFunction ([] (const juce::String& t) {
                                                return t.replace (es ("−"), "-").replaceCharacter (',', '.').getFloatValue();
                                            }));
    }

    std::unique_ptr<APF> faderParam (const char* id, const char* name, float def)
    {
        juce::NormalisableRange<float> r (kFaderMinDb, 6.0f, 0.01f);
        r.setSkewForCentre (-12.0f);
        return std::make_unique<APF> (pid (id), es (name), r, def,
                                      Attr().withLabel ("dB")
                                            .withStringFromValueFunction ([] (float v, int) { return formatFader (v) + " dB"; })
                                            .withValueFromStringFunction ([] (const juce::String& t) {
                                                if (t.contains (es ("∞")) || t.containsIgnoreCase ("inf")) return kFaderMinDb;
                                                return t.replace (es ("−"), "-").replaceCharacter (',', '.').getFloatValue();
                                            }));
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (dbParam (id::entrada, "Entrada", -12.0f, 12.0f, 0.0f));
    l.add (dbParam (id::densidad, "Densidad", 0.0f, 6.0f, 0.0f));
    l.add (dbParam (id::techo, "Techo", -36.0f, 0.0f, -1.0f));
    l.add (std::make_unique<APF> (pid (id::suavizado), es ("Smooth"), juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f,
                                  Attr().withLabel ("%").withStringFromValueFunction ([] (float v, int) {
                                      return juce::String (juce::roundToInt (v)) + " %"; })));
    l.add (std::make_unique<APC> (pid (id::modo), es ("Curva de recorte"),
                                  juce::StringArray { es ("Suave"), es ("Medio"), es ("Duro"), es ("Sin recorte") }, 1));
    l.add (dbParam (id::salida, "Salida", -12.0f, 12.0f, 0.0f));
    l.add (std::make_unique<APB> (pid (id::enlaceES), es ("Compensación de entrada"), false));
    l.add (faderParam (id::humedo, "Procesada", 0.0f));
    l.add (faderParam (id::seco, "Original", kFaderMinDb));
    l.add (std::make_unique<APB> (pid (id::enlaceMezcla), es ("Enlace de mezcla"), true));
    l.add (std::make_unique<APC> (pid (id::tono), es ("Atenuación fija de agudos"),
                                  juce::StringArray { es ("Plano"), es ("Seda"), es ("Terciopelo") }, 0));
    l.add (std::make_unique<APB> (pid (id::techoEstricto), es ("Limitador True Peak"), true));
    l.add (std::make_unique<APB> (pid (id::suavExt), es ("Rango de Smooth −3 dB"), false));
    l.add (std::make_unique<APB> (pid (id::gananciaUnitaria), es ("Compensación de Densidad"), false));
    l.add (std::make_unique<APC> (pid (id::calidad), es ("Oversampling"),
                                  juce::StringArray { es ("1×"), es ("2×"), es ("4×"), es ("8×") }, 1));
    l.add (std::make_unique<APC> (pid (id::calidadOffline), es ("Oversampling al exportar"),
                                  juce::StringArray { es ("Igual"), es ("1×"), es ("2×"), es ("4×"), es ("8×") }, 4));
    l.add (std::make_unique<APC> (pid (id::fase), es ("Fase del filtrado"),
                                  juce::StringArray { es ("Lineal"), es ("Mínima") }, 0));
    return l;
}

void migrateState (juce::ValueTree& state)
{
    const int v = (int) state.getProperty ("version", 0);
    if (v < 1)
    {
        // versión 0 (prototipos): sin cambios de significado; solo se marca la versión.
    }
    state.setProperty ("version", kStateVersion, nullptr);
}
} // namespace clip5::params
