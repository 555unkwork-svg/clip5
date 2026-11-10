// Herramienta de desarrollo: alimenta el procesador con música sintética durante N segundos,
// abre el editor fuera de pantalla y guarda una captura PNG (para documentación y revisión de UI).
// Uso: clip5_snapshot salida.png [escala] [preset] [tema 0-9] [panel]
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

static float synth (int i, double fs, juce::Random& rng)
{
    const double beat = fs * 0.5, pos = std::fmod (i, beat) / fs, t = i / fs;
    double s = 0.9 * std::exp (-pos * 18) * std::sin (2 * juce::MathConstants<double>::pi * (50 + 120 * std::exp (-pos * 40)) * pos);
    if (((int) (i / beat)) % 2 == 1) s += 0.45 * std::exp (-pos * 25) * (rng.nextFloat() * 2 - 1);
    s += 0.3 * std::sin (2 * juce::MathConstants<double>::pi * 55 * t) + 0.12 * std::sin (2 * juce::MathConstants<double>::pi * 440 * t);
    return (float) (0.6 * s);
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String out = argc > 1 ? argv[1] : "clip5.png";
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 1.0f;
    const int preset = argc > 3 ? std::atoi (argv[3]) : 1;
    if (argc > 4) clip5::ui::Themes::get().setIndex (std::atoi (argv[4]), false);
    const juce::String panel = argc > 5 ? juce::String::fromUTF8 (argv[5]) : juce::String();

    std::unique_ptr<clip5::Clip5Processor> p (dynamic_cast<clip5::Clip5Processor*> (createPluginFilter()));
    const double fs = 48000;
    p->prepareToPlay (fs, 512);
    p->presets.loadFactory (preset);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
    ed->setSize ((int) (1040 * scale), (int) (640 * scale));
    juce::AudioBuffer<float> buf (2, 480);
    juce::MidiBuffer midi;
    juce::Random rng (3);
    int n = 0;
    for (int block = 0; block < 900; ++block) // 9 s
    {
        for (int i = 0; i < 480; ++i) { const float v = synth (n++, fs, rng); buf.setSample (0, i, v); buf.setSample (1, i, v * 0.9f); }
        p->processBlock (buf, midi);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }
    juce::MessageManager::getInstance()->runDispatchLoopUntil (200);
    if (auto* ce = dynamic_cast<clip5::Clip5Editor*> (ed.get())) { if (panel.isNotEmpty()) ce->showPanel (panel); ce->refresh(); }
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    std::printf ("captura: %s (%dx%d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset();
    p->releaseResources();
    return 0;
}
