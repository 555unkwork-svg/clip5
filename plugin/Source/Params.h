// clip5 — parámetros (IDs estables; nombres visibles en español).
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace clip5::params
{
// Versión del estado guardado. Subirla cuando cambie el significado de algún parámetro y
// agregar la migración en migrateState().
constexpr int kStateVersion = 1;

namespace id
{
    inline constexpr auto entrada = "entrada";
    inline constexpr auto salida = "salida";
    inline constexpr auto enlaceES = "enlace_es";
    inline constexpr auto techo = "techo";
    inline constexpr auto densidad = "densidad";
    inline constexpr auto modo = "modo";
    inline constexpr auto suavizado = "suavizado";
    inline constexpr auto suavExt = "suavizado_ext";
    inline constexpr auto tono = "tono";
    inline constexpr auto techoEstricto = "techo_estricto";
    inline constexpr auto gananciaUnitaria = "ganancia_unitaria";
    inline constexpr auto humedo = "humedo";
    inline constexpr auto seco = "seco";
    inline constexpr auto enlaceMezcla = "enlace_mezcla";
    inline constexpr auto calidad = "calidad";
    inline constexpr auto calidadOffline = "calidad_offline";
    inline constexpr auto fase = "fase";
}

constexpr float kFaderMinDb = -60.0f; // posición mínima = −∞

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

/** Texto en UTF-8 (todas las cadenas de la interfaz están en español). */
inline juce::String es (const char* utf8) { return juce::String::fromUTF8 (utf8); }

juce::String formatDb (float db, int decimals = 1);
juce::String formatFader (float db);

/** Migración de estados guardados por versiones anteriores. */
void migrateState (juce::ValueTree& state);
} // namespace clip5::params
