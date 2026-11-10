#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace clip5
{
using namespace params;

// =====================================================================================
MeterHub::MeterHub (Engine& e) : engine (e), hist ((size_t) kHistory)
{
    lufsIn.reset();
    lufsOut.reset();
    startTimerHz (30);
}

void MeterHub::resetReadouts()
{
    peakInMax = peakOutMax = tpOutMax = -120;
    lufsIn.reset();
    lufsOut.reset();
}

void MeterHub::timerCallback()
{
    MeterFrame f;
    bool any = false;
    float inPk[2] { 0, 0 }, outPk[2] { 0, 0 };
    while (engine.meterFifo().pop (f))
    {
        any = true;
        const int nc = std::max (1, f.channels);
        for (int c = 0; c < 2; ++c)
        {
            const int src = std::min (c, nc - 1);
            inPk[c] = std::max (inPk[c], f.inPeak[src]);
            outPk[c] = std::max (outPk[c], f.outPeak[src]);
        }
        Column col;
        col.inDb = gainToDb (std::max (f.inPeak[0], f.inPeak[nc - 1]), -120.0f);
        col.outDb = gainToDb (std::max (f.outPeak[0], f.outPeak[nc - 1]), -120.0f);
        col.preDb = f.preClipDb;
        col.clipDb = f.clipDb;
        col.densDb = f.densityDb;
        col.ceilDb = f.ceilingDb;
        hist[(size_t) head] = col;
        head = (head + 1) % kHistory;
        filled = std::min (filled + 1, kHistory);
        ++total;

        lufsIn.add (f.kIn, f.samples);
        lufsOut.add (f.kOut, f.samples);
        peakInMax = std::max (peakInMax, col.inDb);
        peakOutMax = std::max (peakOutMax, col.outDb);
        tpOutMax = std::max (tpOutMax, gainToDb (f.outTruePeak, -120.0f));
        clipTick = std::max (clipTick, f.clipDb);
        densNow = f.densityDb;
    }
    // lectura de recorte con retención de 1 s (una trama sola dura 10 ms: ilegible)
    if (clipTick >= clipNow) { clipNow = clipTick; clipHold = 30; }
    else if (--clipHold <= 0) clipNow = clipTick;
    clipTick = 0.0f;
    // barras con caída de 24 dB/s y retención de pico de 1,5 s
    const float fall = 24.0f / 30.0f;
    for (int c = 0; c < 2; ++c)
    {
        const float ni = any ? gainToDb (inPk[c], -120.0f) : -120.0f;
        const float no = any ? gainToDb (outPk[c], -120.0f) : -120.0f;
        barIn[c] = std::max (ni, barIn[c] - fall);
        barOut[c] = std::max (no, barOut[c] - fall);
        auto hold = [&] (float& h, int& t, float v) {
            if (v >= h) { h = v; t = 45; }
            else if (--t <= 0) h = std::max (v, h - fall);
        };
        hold (holdIn[c], holdTimer[c], barIn[c]);
        hold (holdOut[c], holdTimer[2 + c], barOut[c]);
    }
}

// =====================================================================================
Clip5Processor::Clip5Processor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Entrada", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Salida", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "CLIP5", createLayout()),
      presets (apvts),
      hub (engine)
{
    auto get = [this] (const char* pid) { return apvts.getRawParameterValue (pid); };
    raw = { get (id::entrada), get (id::salida), get (id::enlaceES), get (id::techo), get (id::densidad),
            get (id::modo), get (id::suavizado), get (id::suavExt), get (id::tono), get (id::techoEstricto),
            get (id::gananciaUnitaria), get (id::humedo), get (id::seco), get (id::calidad),
            get (id::calidadOffline), get (id::fase) };
    apvts.state.setProperty ("version", kStateVersion, nullptr);
    engine.setParams (readParams());
}

Clip5Processor::~Clip5Processor()
{
    engine.stopBackgroundWorker();
}

EngineParams Clip5Processor::readParams() const noexcept
{
    EngineParams p;
    p.inputDb = raw.entrada->load();
    p.outputDb = raw.salida->load();
    p.ioLink = raw.enlaceES->load() > 0.5f;
    p.ceilingDb = raw.techo->load();
    p.densityDb = raw.densidad->load();
    p.mode = (ClipMode) juce::jlimit (0, 3, (int) raw.modo->load());
    p.suavizado = raw.suavizado->load() * 0.01f;
    p.suavizadoExtendido = raw.suavExt->load() > 0.5f;
    p.tono = juce::jlimit (0, 2, (int) raw.tono->load());
    p.strictCeiling = raw.techoEstricto->load() > 0.5f;
    p.unitGain = raw.gananciaUnitaria->load() > 0.5f;
    const float hum = raw.humedo->load(), sec = raw.seco->load();
    p.wetDb = hum <= kFaderMinDb + 0.05f ? kSilenceDb : hum;
    p.dryDb = sec <= kFaderMinDb + 0.05f ? kSilenceDb : sec;
    p.qualityRealtime = juce::jlimit (0, 3, (int) raw.calidad->load());
    const int off = (int) raw.calidadOffline->load();
    p.qualityOffline = off == 0 ? -1 : off - 1;
    p.phase = juce::jlimit (0, 1, (int) raw.fase->load());
    return p;
}

void Clip5Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    sr = sampleRate;
    engine.setParams (readParams());
    engine.prepare (sampleRate, juce::jmax (1, getTotalNumOutputChannels()));
    engine.startBackgroundWorker();
    setLatencySamples (engine.latencySamples());
    bypassDelay.setSize (2, 4096);
    bypassDelay.clear();
    bypassPos = 0;
}

void Clip5Processor::releaseResources()
{
    engine.stopBackgroundWorker();
}

bool Clip5Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void Clip5Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto t0 = juce::Time::getHighResolutionTicks();
    const int n = buffer.getNumSamples();
    const int nch = juce::jmin (2, buffer.getNumChannels());
    for (int c = nch; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (n == 0 || nch == 0) return;

    double seconds = -1.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (pos->getIsPlaying())
                if (auto t = pos->getTimeInSeconds()) seconds = *t;

    engine.setParams (readParams());
    float* ch[2] = { buffer.getWritePointer (0), buffer.getWritePointer (nch > 1 ? 1 : 0) };
    engine.process (ch, n, ! isNonRealtime(), seconds);

    if (engine.latencySamples() != getLatencySamples()) setLatencySamples (engine.latencySamples());

    const double el = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - t0);
    const float load = (float) (100.0 * el / (n / sr));
    cpuLoad.store (cpuLoad.load() * 0.97f + load * 0.03f);
}

void Clip5Processor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // bypass con la misma latencia: sin saltos de alineación al activarlo en la sesión
    const int L = juce::jlimit (0, bypassDelay.getNumSamples() - 1, getLatencySamples());
    const int size = bypassDelay.getNumSamples();
    const int nch = juce::jmin (2, buffer.getNumChannels());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        for (int c = 0; c < nch; ++c)
        {
            float* d = bypassDelay.getWritePointer (c);
            d[bypassPos] = buffer.getSample (c, i);
            buffer.setSample (c, i, d[(bypassPos - L + size) % size]);
        }
        bypassPos = (bypassPos + 1) % size;
    }
}

// ---------------------------------------------------------------------------------- estado
juce::ValueTree Clip5Processor::captureState()
{
    return apvts.copyState();
}

void Clip5Processor::restoreState (const juce::ValueTree& t)
{
    if (t.isValid()) apvts.replaceState (t.createCopy());
}

void Clip5Processor::abSelect (char slot)
{
    if (linked)
    {
        slotA = captureState();
        slotB = captureState();
        linked = false;
        active = slot;
        return;
    }
    if (slot == active) return;
    (active == 'A' ? slotA : slotB) = captureState();
    restoreState (slot == 'A' ? slotA : slotB);
    active = slot;
}

void Clip5Processor::abCopyAndRelink()
{
    linked = true;
    slotA = slotB = juce::ValueTree();
}

void Clip5Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("version", kStateVersion, nullptr);
    state.setProperty ("preset", presets.currentName(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void Clip5Processor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    migrateState (tree);
    presets.setCurrentName (tree.getProperty ("preset", presets.currentName()).toString());
    apvts.replaceState (tree);
    linked = true;
}

juce::AudioProcessorEditor* Clip5Processor::createEditor()
{
    return new Clip5Editor (*this);
}
} // namespace clip5

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new clip5::Clip5Processor();
}
