// clip5 — AudioProcessor: puente entre APVTS y el motor DSP, A/B, presets y medición para la UI.
#pragma once

#include "Params.h"
#include "Presets.h"
#include "clip5/Engine.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace clip5
{
/** Datos de medición ya integrados para la UI (solo hilo de mensajes). */
class MeterHub : private juce::Timer
{
public:
    struct Column { float inDb = -120, outDb = -120, preDb = -120, clipDb = 0, densDb = 0, ceilDb = 0; };
    static constexpr int kHistory = 2048;

    explicit MeterHub (Engine& e);
    ~MeterHub() override { stopTimer(); }

    // historial del display (una columna = 10 ms)
    const Column& column (int ago) const noexcept { return hist[(size_t) ((head - 1 - ago + kHistory * 4) % kHistory)]; }
    int columnsAvailable() const noexcept { return filled; }
    uint64_t totalColumns() const noexcept { return total; }

    // barras (dBFS) y retenciones
    float barIn[2] { -120, -120 }, barOut[2] { -120, -120 }, holdIn[2] { -120, -120 }, holdOut[2] { -120, -120 };
    float peakInMax = -120, peakOutMax = -120, tpOutMax = -120;
    float clipNow = 0, densNow = 0;
    LoudnessAccumulator lufsIn, lufsOut;

    void resetReadouts();

private:
    Engine& engine;
    std::vector<Column> hist;
    int head = 0, filled = 0;
    uint64_t total = 0;
    int holdTimer[4] {};
    float clipTick = 0.0f;
    int clipHold = 0;
    void timerCallback() override;
};

class Clip5Processor : public juce::AudioProcessor,
                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    Clip5Processor();
    ~Clip5Processor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "clip5"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    Engine engine;
    MeterHub hub;

    // ---- A/B (hilo de mensajes)
    bool abLinked() const noexcept { return linked; }
    char abActive() const noexcept { return active; }
    void abSelect (char slot);
    void abCopyAndRelink();

    float cpuLoadPercent() const noexcept { return cpuLoad.load(); }

private:
    struct Raw
    {
        std::atomic<float>* entrada; std::atomic<float>* salida; std::atomic<float>* enlaceES;
        std::atomic<float>* techo; std::atomic<float>* densidad; std::atomic<float>* modo;
        std::atomic<float>* suavizado; std::atomic<float>* suavExt; std::atomic<float>* tono;
        std::atomic<float>* techoEstricto; std::atomic<float>* gananciaUnitaria;
        std::atomic<float>* humedo; std::atomic<float>* seco;
        std::atomic<float>* calidad; std::atomic<float>* calidadOffline; std::atomic<float>* fase;
    } raw {};

    EngineParams readParams() const noexcept;
    void parameterChanged (const juce::String&, float) override {}

    bool linked = true;
    char active = 'A';
    juce::ValueTree slotA, slotB;
    juce::ValueTree captureState();
    void restoreState (const juce::ValueTree& t);

    juce::AudioBuffer<float> bypassDelay;
    int bypassPos = 0;
    std::atomic<float> cpuLoad { 0.0f };
    double sr = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Clip5Processor)
};
} // namespace clip5
