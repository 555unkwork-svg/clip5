// clip5 — presets de fábrica (en español) y del usuario.
// Los presets no tocan la calidad ni la fase: son preferencias del ingeniero, no del sonido.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace clip5
{
class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    int numFactory() const noexcept;
    juce::String factoryName (int i) const;
    void loadFactory (int i);

    juce::File userFolder() const;
    juce::Array<juce::File> userPresets() const;
    bool saveUser (const juce::String& name);
    bool loadUser (const juce::File& f);

    juce::String currentName() const { return current; }
    void setCurrentName (const juce::String& n) { current = n; }

    /** Lista unificada (fábrica + usuario) para navegar con ‹ ›. */
    int count() const;
    juce::String nameAt (int i) const;
    void loadAt (int i);
    int indexOfCurrent() const;

    static bool isExcludedFromPresets (const juce::String& paramId);

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String current;
    void setParam (const char* id, float value);
    void resetToDefaults();
};
} // namespace clip5
