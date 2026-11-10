#include "PluginEditor.h"

namespace clip5
{
using namespace ui;
using params::es;
using params::formatDb;
namespace id = params::id;

// =====================================================================================
//  Panel Avanzado (plegable, superpuesto sobre el display)
// =====================================================================================
class Clip5Editor::Advanced : public juce::Component
{
public:
    explicit Advanced (juce::AudioProcessorValueTreeState& s)
        : tono (s, id::tono, { es ("Plano"), es ("Seda"), es ("Terciopelo") }),
          estricto (s, id::techoEstricto, es ("Activado"), es ("Desactivado")),
          suavExt (s, id::suavExt, es ("Hasta −3 dB"), es ("Hasta −2 dB")),
          enlace (s, id::enlaceES, es ("Activado"), es ("Desactivado")),
          offline (s, id::calidadOffline, { es ("Igual"), es ("Eco"), es ("Normal"), es ("Alta"), es ("Máx.") }),
          fase (s, id::fase, { es ("Lineal"), es ("Mínima") }),
          unitaria (s, id::gananciaUnitaria, es ("Activada"), es ("Desactivada"))
    {
        tono.setTooltip (es ("Caída suave de agudos incorporada al filtro de decimación (sin costo de CPU). "
                             "Plano bajo 1 kHz."));
        estricto.setTooltip (es ("Limitador true peak al final de la cadena: la salida nunca supera el techo "
                                 "(medición BS.1770-4), incluso con mezcla paralela o Salida positiva."));
        suavExt.setTooltip (es ("Profundidad máxima del Suavizado."));
        enlace.setTooltip (es ("Subir la entrada baja en la misma cantidad la salida del camino procesado: el volumen "
                               "percibido se mantiene mientras se ajusta el recorte. SALIDA queda como offset global "
                               "(también con Shift+arrastrar ENTRADA)."));
        offline.setTooltip (es ("Calidad usada al exportar (render sin tiempo real)."));
        fase.setTooltip (es ("Lineal: sin distorsión de fase, mezcla paralela perfecta. Mínima: latencia de pocas "
                             "muestras; la mezcla con SECO puede colorear agudos."));
        unitaria.setTooltip (es ("Resta la ganancia nominal de Densidad para comparar a igual volumen."));
        for (juce::Component* c : std::initializer_list<juce::Component*> { &tono, &estricto, &suavExt, &enlace, &offline, &fase, &unitaria })
            addAndMakeVisible (c);
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (col::sup);
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (col::lin);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        g.setFont (Fonts::sans (10.5f, true));
        g.setColour (col::txt2);
        for (auto& [c, t] : titles) g.drawText (t, c->getBounds().translated (0, -18).withHeight (14), juce::Justification::left);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (16, 14);
        const int colW = r.getWidth() / 4;
        auto top = r.removeFromTop (r.getHeight() / 2);
        auto place = [] (juce::Rectangle<int> area, juce::Component& c, int w, int h) {
            c.setBounds (area.reduced (0, 0).withTrimmedTop (20).withSize (w, h));
        };
        place (top.removeFromLeft (colW), tono, colW - 20, 28);
        place (top.removeFromLeft (colW), estricto, colW - 20, 24);
        place (top.removeFromLeft (colW), suavExt, colW - 20, 24);
        place (top.removeFromLeft (colW), enlace, colW - 20, 24);
        place (r.removeFromLeft (colW), offline, colW - 20, 28);
        place (r.removeFromLeft (colW), fase, colW - 20, 28);
        place (r.removeFromLeft (colW), unitaria, colW - 20, 24);
        titles = { { &tono, es ("TONO") }, { &estricto, es ("TECHO ESTRICTO") }, { &suavExt, es ("SUAVIZADO EXTENDIDO") },
                   { &enlace, es ("ENLACE E/S") }, { &offline, es ("CALIDAD AL EXPORTAR") }, { &fase, es ("FASE") },
                   { &unitaria, es ("GANANCIA UNITARIA") } };
    }

private:
    Segmented tono;
    Switch estricto, suavExt, enlace;
    Segmented offline, fase;
    Switch unitaria;
    std::vector<std::pair<juce::Component*, juce::String>> titles;
};

// =====================================================================================
//  Contenido (tamaño base fijo; se escala con una transformación)
// =====================================================================================
class Clip5Editor::Content : public juce::Component
{
public:
    explicit Content (Clip5Editor& e)
        : ed (e), proc (e.proc), s (e.proc.apvts),
          entrada (s, id::entrada, es ("ENTRADA"), Knob::Style::Menta, true, [] (float v) { return formatDb (v) + " dB"; }),
          densidad (s, id::densidad, es ("DENSIDAD"), Knob::Style::Menta, false, [] (float v) { return formatDb (v) + " dB"; }),
          techo (s, id::techo, es ("TECHO"), Knob::Style::Marfil, false, [] (float v) { return formatDb (v) + " dB"; }),
          suavizado (s, id::suavizado, es ("SUAVIZADO"), Knob::Style::Menta55, false, [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; }),
          salida (s, id::salida, es ("SALIDA"), Knob::Style::Menta, true, [] (float v) { return formatDb (v) + " dB"; }),
          modos (s),
          humedo (s, id::humedo, es ("HÚMEDO")),
          seco (s, id::seco, es ("SECO")),
          display (e.proc.hub),
          meters (e.proc.hub),
          clock (e.proc.engine),
          advanced (s),
          calidad (s, id::calidad, { es ("Eco"), es ("Normal"), es ("Alta"), es ("Máx.") }),
          presetBtn ({}, es ("Presets de fábrica y del usuario")),
          prevBtn (es ("‹"), es ("Preset anterior")), nextBtn (es ("›"), es ("Preset siguiente")),
          aBtn ("A", es ("Estado A. Al pulsar A o B se desenlazan; los cambios aplican solo al activo.")),
          bBtn ("B", es ("Estado B. Al pulsar A o B se desenlazan; los cambios aplican solo al activo.")),
          copyBtn (es ("⇄"), es ("Copiar el estado activo al otro y volver a enlazar A y B")),
          undoBtn (es ("↶"), es ("Deshacer (Ctrl/Cmd+Z)")), redoBtn (es ("↷"), es ("Rehacer (Ctrl/Cmd+Shift+Z)")),
          gearBtn (es ("⚙"), es ("Ajustes: escala de la interfaz y frecuencia del display")),
          advBtn (es ("Avanzado ▴"), es ("Tono, Techo estricto, Enlace E/S, calidad al exportar, fase y ganancia unitaria")),
          linkBtn (es ("⛓"), es ("Enlazados: crossfade de potencia constante entre HÚMEDO y SECO"))
    {
        entrada.setTooltip (es ("Ganancia de entrada del camino procesado (±12 dB); no afecta a SECO. Shift = ajuste fino; "
                                "con Enlace E/S, Shift+arrastrar ajusta el offset de salida. Doble clic: escribir el valor. "
                                "Ctrl/Cmd+clic: valor por defecto."));
        densidad.setTooltip (es ("Ganancia no lineal instantánea: +G dB a bajo nivel que llega a 0 dB exactamente en el "
                                 "techo. Sin ataque ni release."));
        techo.setTooltip (es ("Nivel máximo (0 a −36 dBFS). Arrastra consigo las rodillas de Densidad y Suavizado."));
        suavizado.setTooltip (es ("Contorno dinámico de medios-agudos (hasta −2 dB, o −3 dB en modo extendido) que solo "
                                  "actúa cerca del techo o por encima. Doma los armónicos del recorte."));
        salida.setTooltip (es ("Ganancia de salida (±12 dB). Con Enlace E/S es el offset sobre la compensación automática."));
        humedo.setTooltip (es ("Nivel de la señal procesada (−∞ a +6 dB). Doble clic: 0 dB."));
        seco.setTooltip (es ("Nivel de la señal original (antes de ENTRADA), compensada en latencia (−∞ a +6 dB)."));
        calidad.setTooltip (es ("Calidad en tiempo real: Eco 1x, Normal 2x, Alta 4x, Máxima 8x (todas con ADAA). "
                                "La calidad al exportar se elige en Avanzado."));
        salida.compact = true;
        entrada.shiftTarget = [this]() -> ParamControl* { return linkedIO() ? &salida : nullptr; };

        for (juce::Component* c : std::initializer_list<juce::Component*> {
                 &display, &entrada, &densidad, &techo, &suavizado, &modos, &meters, &clock, &humedo, &seco,
                 &salida, &calidad, &presetBtn, &prevBtn, &nextBtn, &aBtn, &bBtn, &copyBtn, &undoBtn, &redoBtn,
                 &gearBtn, &advBtn, &linkBtn })
            addAndMakeVisible (c);
        addChildComponent (advanced);

        linkAtt = std::make_unique<juce::ButtonParameterAttachment> (*s.getParameter (id::enlaceMezcla), linkBtn, s.undoManager);
        linkBtn.setClickingTogglesState (true);
        humedo.onUserChange = [this] (float db) { if (linkBtn.getToggleState()) seco.setFromLink (complement (db)); };
        seco.onUserChange = [this] (float db) { if (linkBtn.getToggleState()) humedo.setFromLink (complement (db)); };

        advBtn.onClick = [this] { toggleAdvanced(); };
        aBtn.onClick = [this] { proc.abSelect ('A'); };
        bBtn.onClick = [this] { proc.abSelect ('B'); };
        copyBtn.onClick = [this] { proc.abCopyAndRelink(); };
        undoBtn.onClick = [this] { proc.undo.undo(); };
        redoBtn.onClick = [this] { proc.undo.redo(); };
        prevBtn.onClick = [this] { stepPreset (-1); };
        nextBtn.onClick = [this] { stepPreset (1); };
        presetBtn.onClick = [this] { showPresetMenu(); };
        gearBtn.onClick = [this] { showSettingsMenu(); };
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (col::fondo);
        // barra superior
        auto bar = getLocalBounds().removeFromTop (48);
        g.setColour (col::sup);
        g.fillRect (bar);
        g.setColour (col::lin);
        g.drawHorizontalLine (47, 0.0f, (float) getWidth());
        // logo: "clip" + "5" en bloque menta con muesca (motivo de marca)
        const auto logoFont = Fonts::sans (21.0f, true);
        g.setFont (logoFont);
        g.setColour (col::txt);
        const float clipW = juce::GlyphArrangement::getStringWidth (logoFont, "clip");
        g.drawText ("clip", 16, 10, 60, 28, juce::Justification::left);
        auto five = juce::Rectangle<float> (16.0f + clipW + 2.0f, 12.0f, 17.0f, 24.0f);
        g.setColour (col::menta);
        g.fillRoundedRectangle (five, 3.5f);
        g.setColour (col::sup);
        g.fillRect (five.getRight() - 4.0f, five.getY() + 6.0f, 4.0f, 2.0f);
        g.setColour (col::fondo);
        g.drawText ("5", five.translated (0, -0.5f), juce::Justification::centred);
        // separadores, etiquetas
        g.setColour (col::lin);
        for (int x : sepX) g.fillRect (x, 13, 1, 22);
        g.setFont (Fonts::sans (11.5f));
        g.setColour (col::txt2);
        g.drawText (abText, abLabel, juce::Justification::centredLeft);
        g.setFont (Fonts::mono (11.0f));
        g.drawText (cpuText, cpuLabel, juce::Justification::centredLeft);
        // columna derecha
        g.setColour (col::lin);
        g.drawVerticalLine (getWidth() - 237, 48.0f, (float) getHeight());
        // caja de mezcla
        auto mix = mixBox.toFloat();
        g.setColour (col::sup);
        g.fillRoundedRectangle (mix, 8.0f);
        g.setColour (col::lin);
        g.drawRoundedRectangle (mix.reduced (0.5f), 8.0f, 1.0f);
        g.setFont (Fonts::sans (10.5f, true));
        g.setColour (col::txt2);
        g.drawText (es ("MEZCLA · SALIDA"), mixBox.reduced (10, 8).removeFromTop (14), juce::Justification::left);
        g.drawText (es ("MODO DE RECORTE"), modeTitle, juce::Justification::left);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        auto bar = r.removeFromTop (48).reduced (14, 10);
        bar.removeFromLeft (76);
        presetBtn.setBounds (bar.removeFromLeft (210));
        bar.removeFromLeft (6);
        prevBtn.setBounds (bar.removeFromLeft (28));
        bar.removeFromLeft (4);
        nextBtn.setBounds (bar.removeFromLeft (28));
        sepX[0] = bar.getX() + 10; bar.removeFromLeft (20);
        aBtn.setBounds (bar.removeFromLeft (28)); bar.removeFromLeft (4);
        bBtn.setBounds (bar.removeFromLeft (28)); bar.removeFromLeft (4);
        copyBtn.setBounds (bar.removeFromLeft (28)); bar.removeFromLeft (8);
        abLabel = bar.removeFromLeft (80);
        sepX[1] = bar.getX() + 4; bar.removeFromLeft (12);
        undoBtn.setBounds (bar.removeFromLeft (28)); bar.removeFromLeft (4);
        redoBtn.setBounds (bar.removeFromLeft (28));
        gearBtn.setBounds (bar.removeFromRight (28));
        bar.removeFromRight (8);
        cpuLabel = bar.removeFromRight (88);
        bar.removeFromRight (6);
        calidad.setBounds (bar.removeFromRight (236));

        auto right = r.removeFromRight (236).reduced (12, 12);
        meters.setBounds (right.removeFromTop (196));
        right.removeFromTop (10);
        clock.setBounds (right.removeFromTop (84));
        right.removeFromTop (10);
        mixBox = right;
        auto mx = right.reduced (10).withTrimmedTop (22);
        humedo.setBounds (mx.removeFromLeft (46));
        linkBtn.setBounds (mx.removeFromLeft (30).withSizeKeepingCentre (26, 26));
        seco.setBounds (mx.removeFromLeft (46));
        salida.setBounds (mx.reduced (2, 0).withSizeKeepingCentre (mx.getWidth() - 4, 110));

        auto controls = r.removeFromBottom (196);
        display.setBounds (r.reduced (12, 12).withTrimmedRight (0));
        advanced.setBounds (display.getBounds().withTop (display.getBottom() - 170));
        auto c = controls.reduced (12, 10);
        for (auto* k : { &entrada, &densidad, &techo, &suavizado })
        {
            k->setBounds (c.removeFromLeft (112).withTrimmedTop (8));
            c.removeFromLeft (4);
        }
        auto m = c.withTrimmedLeft (8);
        modeTitle = m.removeFromTop (22).withTrimmedTop (6);
        modos.setBounds (m.removeFromTop (100));
        m.removeFromTop (10);
        advBtn.setBounds (m.removeFromTop (30).withWidth (110));
    }

    void tick()
    {
        const float ceilDb = proc.apvts.getRawParameterValue (id::techo)->load();
        const float dens = proc.apvts.getRawParameterValue (id::densidad)->load();
        const int mode = (int) proc.apvts.getRawParameterValue (id::modo)->load();
        display.tick (ceilDb);
        display.transfer.set (dens, mode);
        modos.setDensity (dens);
        meters.tick (ceilDb);
        clock.tick();

        const bool link = linkedIO();
        salida.nameOverride = link ? es ("SALIDA (offset)") : juce::String();
        const juce::String ab = proc.abLinked() ? es ("A = B") : es ("editando ") + juce::String::charToString (proc.abActive());
        aBtn.setToggleState (! proc.abLinked() && proc.abActive() == 'A', juce::dontSendNotification);
        bBtn.setToggleState (! proc.abLinked() && proc.abActive() == 'B', juce::dontSendNotification);
        undoBtn.setEnabled (proc.undo.canUndo());
        redoBtn.setEnabled (proc.undo.canRedo());
        presetBtn.setButtonText (proc.presets.currentName());
        if (++slow % 10 == 0)
        {
            const auto t = "CPU " + juce::String (proc.cpuLoadPercent(), 1).replaceCharacter ('.', ',') + " %";
            if (t != cpuText || ab != abText) { cpuText = t; abText = ab; repaint (cpuLabel); repaint (abLabel); }
        }
    }

private:
    Clip5Editor& ed;
    Clip5Processor& proc;
    juce::AudioProcessorValueTreeState& s;
    Knob entrada, densidad, techo, suavizado, salida;
    ModeSelector modos;
    Fader humedo, seco;
    HistoryDisplay display;
    MeterPanel meters;
    ClipClockView clock;
    Advanced advanced;
    Segmented calidad;
    FlatButton presetBtn, prevBtn, nextBtn, aBtn, bBtn, copyBtn, undoBtn, redoBtn, gearBtn, advBtn, linkBtn;
    std::unique_ptr<juce::ButtonParameterAttachment> linkAtt;
    juce::Rectangle<int> abLabel, calLabel, cpuLabel, mixBox, modeTitle;
    int sepX[2] {};
    juce::String abText, cpuText;
    int slow = 0;

    bool linkedIO() const { return proc.apvts.getRawParameterValue (id::enlaceES)->load() > 0.5f; }

    static float complement (float db)
    {
        // potencia constante: a² + b² = 1 (ambos ≤ 0 dB)
        if (db <= params::kFaderMinDb + 0.05f) return 0.0f;
        const float a = std::min (1.0f, juce::Decibels::decibelsToGain (db));
        const float b = std::sqrt (std::max (0.0f, 1.0f - a * a));
        return b < 0.001f ? params::kFaderMinDb : std::max (params::kFaderMinDb, juce::Decibels::gainToDecibels (b));
    }

    void toggleAdvanced()
    {
        const bool show = ! advanced.isVisible();
        advanced.setVisible (show);
        advBtn.setButtonText (show ? es ("Avanzado ▾") : es ("Avanzado ▴"));
        advBtn.setToggleState (show, juce::dontSendNotification);
    }

    void stepPreset (int d)
    {
        auto& pm = proc.presets;
        const int n = pm.count();
        if (n == 0) return;
        int i = pm.indexOfCurrent();
        i = ((i < 0 ? 0 : i + d) % n + n) % n;
        proc.undo.beginNewTransaction();
        pm.loadAt (i);
    }

    void showPresetMenu()
    {
        auto& pm = proc.presets;
        juce::PopupMenu m;
        m.addSectionHeader (es ("Fábrica"));
        for (int i = 0; i < pm.numFactory(); ++i)
            m.addItem (1 + i, pm.factoryName (i), true, pm.currentName() == pm.factoryName (i));
        const auto user = pm.userPresets();
        if (! user.isEmpty())
        {
            m.addSectionHeader (es ("Usuario"));
            for (int i = 0; i < user.size(); ++i)
                m.addItem (100 + i, user[i].getFileNameWithoutExtension(), true, pm.currentName() == user[i].getFileNameWithoutExtension());
        }
        m.addSeparator();
        m.addItem (900, es ("Guardar preset…"));
        m.addItem (901, es ("Abrir carpeta de presets"));
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn),
                         [this, user] (int r) {
                             auto& p = proc.presets;
                             if (r >= 1 && r < 100) { proc.undo.beginNewTransaction(); p.loadFactory (r - 1); }
                             else if (r >= 100 && r < 900) { proc.undo.beginNewTransaction(); p.loadUser (user[r - 100]); }
                             else if (r == 900) askPresetName();
                             else if (r == 901) { p.userFolder().createDirectory(); p.userFolder().startAsProcess(); }
                         });
    }

    void askPresetName()
    {
        auto* w = new juce::AlertWindow (es ("Guardar preset"), es ("Nombre del preset:"), juce::MessageBoxIconType::NoIcon, this);
        w->setLookAndFeel (&ed.lnf);
        w->addTextEditor ("nombre", proc.presets.currentName());
        w->addButton (es ("Guardar"), 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton (es ("Cancelar"), 0, juce::KeyPress (juce::KeyPress::escapeKey));
        w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r) {
            if (r == 1)
            {
                const auto name = w->getTextEditorContents ("nombre").trim();
                if (name.isNotEmpty()) proc.presets.saveUser (name);
            }
        }), true);
    }

    void showSettingsMenu()
    {
        juce::PopupMenu m, sc, fr;
        for (int pct : { 75, 100, 125, 150, 175, 200 })
            sc.addItem (pct, juce::String (pct) + " %", true, std::abs (ed.scale * 100.0f - (float) pct) < 1.0f);
        fr.addItem (1030, "30 fps", true, ed.fps == 30);
        fr.addItem (1060, "60 fps", true, ed.fps == 60);
        m.addSubMenu (es ("Escala de la interfaz"), sc);
        m.addSubMenu (es ("Frecuencia del display"), fr);
        m.addSeparator();
        m.addItem (2000, es ("clip5 ") + CLIP5_VERSION_STRING + es (" · latencia ") + juce::String (proc.getLatencySamples()) + es (" muestras"), false, false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&gearBtn), [this] (int r) {
            if (r >= 75 && r <= 200) ed.setScale ((float) r / 100.0f);
            else if (r == 1030 || r == 1060) { ed.fps = r - 1000; ed.settings->setValue ("fps", ed.fps); ed.restartTimer(); }
        });
    }
};

// =====================================================================================
Clip5Editor::Clip5Editor (Clip5Processor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    tooltips.setLookAndFeel (&lnf);
    juce::PropertiesFile::Options o;
    o.applicationName = "clip5";
    o.filenameSuffix = ".settings";
    o.folderName = "clip5";
    o.osxLibrarySubFolder = "Application Support";
    settings = std::make_unique<juce::PropertiesFile> (o);
    scale = juce::jlimit (0.75f, 2.0f, (float) settings->getDoubleValue ("escala", 1.0));
    fps = settings->getIntValue ("fps", 30) == 60 ? 60 : 30;

    content = std::make_unique<Content> (*this);
    addAndMakeVisible (*content);
    content->setBounds (0, 0, kW, kH);
    addMouseListener (this, true);
    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setResizeLimits ((int) (kW * 0.75f), (int) (kH * 0.75f), kW * 2, kH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kW / kH);
    setSize ((int) (kW * scale), (int) (kH * scale));
    content->tick();
    restartTimer();
}

Clip5Editor::~Clip5Editor()
{
    stopTimer();
    tooltips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void Clip5Editor::paint (juce::Graphics& g) { g.fillAll (col::fondo); }

void Clip5Editor::resized()
{
    scale = (float) getWidth() / (float) kW;
    content->setTransform (juce::AffineTransform::scale (scale));
    if (settings) settings->setValue ("escala", scale);
}

void Clip5Editor::setScale (float s)
{
    scale = s;
    setSize ((int) std::round (kW * s), (int) std::round (kH * s));
}

void Clip5Editor::restartTimer()
{
    if (isShowing()) startTimerHz (fps); else stopTimer();
}

void Clip5Editor::visibilityChanged() { restartTimer(); }
void Clip5Editor::parentHierarchyChanged() { restartTimer(); }

void Clip5Editor::timerCallback()
{
    if (! isShowing()) { stopTimer(); return; } // ventana oculta: sin repintado
    content->tick();
}

void Clip5Editor::refresh() { content->tick(); }

void Clip5Editor::mouseDown (const juce::MouseEvent&)
{
    proc.undo.beginNewTransaction();
}

bool Clip5Editor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0)) { proc.undo.undo(); return true; }
    if (k == juce::KeyPress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)) { proc.undo.redo(); return true; }
    return false;
}
} // namespace clip5
