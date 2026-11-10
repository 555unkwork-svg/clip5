#include "PluginEditor.h"

namespace clip5
{
using namespace ui;
using params::es;
using params::formatDb;
namespace id = params::id;

namespace
{
    // tarjetas fijas del lienzo base (1040 × 640)
    const juce::Rectangle<int> kLogo { 14, 14, 184, 74 }, kProc { 14, 98, 184, 528 }, kTop { 208, 14, 818, 42 },
        kGraph { 208, 66, 818, 400 }, kMix { 208, 476, 420, 150 }, kSmooth { 638, 476, 196, 150 }, kOs { 844, 476, 182, 150 };
}

// =====================================================================================
class Clip5Editor::Content : public juce::Component
{
public:
    explicit Content (Clip5Editor& e)
        : ed (e), proc (e.proc), s (e.proc.apvts),
          curva (s),
          techo (s, id::techo, "Techo", false, [] (float v) { return formatDb (v) + " dBFS"; }, 15.0f),
          densidad (s, id::densidad, "Densidad", false, [] (float v) { return formatDb (v) + " dB"; }, 13.0f),
          entrada (s, id::entrada, "Entrada", true, [] (float v) { return formatDb (v) + " dB"; }, 12.0f),
          salida (s, id::salida, "Salida", true, [] (float v) { return formatDb (v) + " dB"; }, 12.0f),
          smooth (s, id::suavizado, {}, false, [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; }),
          procesada (s, id::humedo, "Procesada"), original (s, id::seco, "Original"),
          enlace (s, id::enlaceMezcla, "Enlace"),
          oversampling (s, id::calidad, { es ("1×"), es ("2×"), es ("4×"), es ("8×") }),
          display (e.proc.hub),
          meters (e.proc.hub, e.proc.engine), curvePanel (s), advanced (s),
          prevBtn (es ("‹"), es ("Preset anterior")), nextBtn (es ("›"), es ("Preset siguiente")),
          presetBtn ({}, es ("Presets de fábrica y del usuario")),
          undoBtn (es ("↶"), es ("Deshacer (Ctrl/Cmd+Z)")), redoBtn (es ("↷"), es ("Rehacer (Ctrl/Cmd+Shift+Z)")),
          aBtn ("A", es ("Estado A. Al elegir A o B se desenlazan y los cambios aplican solo al activo.")),
          bBtn ("B", es ("Estado B. Al elegir A o B se desenlazan y los cambios aplican solo al activo.")),
          unifyBtn (es ("Unificar A/B"), es ("Conserva el ajuste activo y vuelve a enlazar A y B.")),
          gearBtn (es ("⚙"), es ("Ajustes: Apariencia, escala de la interfaz y frecuencia del gráfico")),
          curveBtn (es ("Curva E/S"), es ("Transferencia estática de Densidad y la curva elegida"), true),
          metersBtn (es ("Medidores"), es ("Picos, pico real (dBTP), LUFS y exceso máximo"), true),
          advBtn (es ("Avanzado"), es ("Oversampling al exportar, fase, Limitador True Peak, atenuación fija, rango de Smooth y compensaciones"), true)
    {
        curva.setTooltip (es ("Curva de recorte"));
        techo.setTooltip (es ("Techo (−36 a 0 dBFS): referencia de la curva, Densidad y Smooth. Con el Limitador True Peak activo, también es el objetivo en dBTP."));
        densidad.setTooltip (es ("Densidad (0 a +6 dB): realce instantáneo a bajo nivel que cae a 0 dB al llegar al Techo."));
        entrada.setTooltip (es ("Entrada (±12 dB): cuánto se empuja la rama procesada contra el Techo. Shift+arrastrar ajusta el offset de Compensación de entrada."));
        salida.setTooltip (es ("Salida (±12 dB): ganancia después de la mezcla paralela y antes del Limitador True Peak."));
        smooth.setTooltip (es ("Smooth (0 a 100 %): reduce dinámicamente medios-agudos cuando la señal se acerca al Techo o lo supera."));
        procesada.setTooltip (es ("Nivel de la rama procesada (−∞ a +6 dB). Doble clic: 0 dB."));
        original.setTooltip (es ("Nivel de la señal original, tomada antes de Entrada y alineada en latencia (−∞ a +6 dB)."));
        enlace.setTooltip (es ("Enlace: al mover un fader, el otro sigue un fundido de potencia constante."));
        oversampling.setTooltip (es ("Oversampling en tiempo real (todas con ADAA). El de exportación se elige en Avanzado."));
        smooth.valueBoxWidth = 70;
        entrada.valueBoxWidth = salida.valueBoxWidth = 70;
        densidad.valueBoxWidth = 76;
        techo.valueBoxWidth = 100;
        entrada.shiftTarget = [this]() -> ParamControl* { return proc.apvts.getRawParameterValue (id::enlaceES)->load() > 0.5f ? &salida : nullptr; };

        for (juce::Component* c : std::initializer_list<juce::Component*> {
                 &display, &curva, &techo, &densidad, &entrada, &salida, &smooth, &procesada, &enlace, &original, &oversampling,
                 &prevBtn, &nextBtn, &presetBtn, &undoBtn, &redoBtn, &aBtn, &bBtn, &unifyBtn, &gearBtn, &curveBtn, &metersBtn, &advBtn, &sticker })
            addAndMakeVisible (c);
        for (juce::Component* c : std::initializer_list<juce::Component*> { &meters, &curvePanel, &advanced, &appearance })
            addChildComponent (c);

        // orden de tabulación: procesamiento → mezcla → barra superior → paneles
        int order = 1;
        for (juce::Component* c : std::initializer_list<juce::Component*> { &curva, &techo, &densidad, &entrada, &salida, &procesada, &enlace, &original,
                                                                            &smooth, &oversampling, &curveBtn, &metersBtn, &advBtn, &prevBtn, &presetBtn,
                                                                            &nextBtn, &undoBtn, &redoBtn, &aBtn, &bBtn, &unifyBtn, &gearBtn })
            c->setExplicitFocusOrder (order++);

        procesada.onUserChange = [this] (float db) { if (linked()) original.setFromLink (complement (db)); };
        original.onUserChange = [this] (float db) { if (linked()) procesada.setFromLink (complement (db)); };

        curveBtn.onClick = [this] { ed.showPanel (curvePanel.isVisible() ? "" : "Curva E/S"); };
        metersBtn.onClick = [this] { ed.showPanel (meters.isVisible() ? "" : "Medidores"); };
        advBtn.onClick = [this] { ed.showPanel (advanced.isVisible() ? "" : "Avanzado"); };
        aBtn.onClick = [this] { proc.abSelect ('A'); };
        bBtn.onClick = [this] { proc.abSelect ('B'); };
        unifyBtn.onClick = [this] { proc.abCopyAndRelink(); };
        undoBtn.onClick = [this] { proc.undo.undo(); };
        redoBtn.onClick = [this] { proc.undo.redo(); };
        prevBtn.onClick = [this] { stepPreset (-1); };
        nextBtn.onClick = [this] { stepPreset (1); };
        presetBtn.onClick = [this] { showPresetMenu(); };
        gearBtn.onClick = [this] { showSettingsMenu(); };
        appearance.onClose = [this] { appearance.setVisible (false); gearBtn.grabKeyboardFocus(); };

        sticker.setInterceptsMouseClicks (true, false);
        sticker.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        sticker.onClick = [this] { ed.showPanel ("Apariencia"); };
        updateSticker();
    }

    void updateSticker()
    {
        sticker.image = Themes::get().sticker (Themes::get().currentIndex());
        sticker.setTooltip (es ("Tema: ") + pal().nombre + es (" · clic para cambiar la apariencia"));
        sticker.repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& c = pal();
        g.fillAll (c.fondo);
        auto card = [&] (juce::Rectangle<int> r, juce::Colour col) { g.setColour (col); g.fillRoundedRectangle (r.toFloat(), 10.0f); };
        card (kLogo, c.sup); card (kProc, c.proc); card (kTop, c.sup); card (kMix, c.sup); card (kSmooth, c.sup); card (kOs, c.sup);

        // logo: CLIP + 5 en el color de selección
        const auto brand = Fonts::brand (34.0f);
        const float wClip = juce::GlyphArrangement::getStringWidth (brand, "CLIP"), w5 = juce::GlyphArrangement::getStringWidth (brand, "5");
        const float x0 = (float) kLogo.getCentreX() - (wClip + w5) * 0.5f;
        g.setFont (brand);
        g.setColour (c.txt);
        g.drawText ("CLIP", juce::Rectangle<float> (x0, 18.0f, wClip + 2, 44.0f), juce::Justification::centredLeft);
        g.setColour (c.sel);
        g.drawText ("5", juce::Rectangle<float> (x0 + wClip, 18.0f, w5 + 4, 44.0f), juce::Justification::centredLeft);
        g.setColour (c.txt2.withAlpha (0.35f));
        g.drawHorizontalLine (66, 30.0f, 182.0f);
        g.setColour (c.txt2);
        g.setFont (Fonts::sans (8.5f, true).withExtraKerningFactor (0.25f));
        g.drawText ("MASTERING CLIPPER", juce::Rectangle<int> (14, 70, 184, 14), juce::Justification::centred);

        auto title = [&] (const juce::String& t, int x, int y) { g.setColour (c.txt); g.setFont (Fonts::brand (11.5f)); g.drawText (t, x, y, 220, 16, juce::Justification::centredLeft); };
        title (es ("CURVA DE RECORTE"), 26, 108);
        g.setColour (c.txt2.withAlpha (0.25f));
        g.drawHorizontalLine (510, 26.0f, 186.0f);
        title ("PRESET", 222, 27);
        title (es ("MEZCLA PARALELA"), 222, 488);
        title ("SMOOTH", 652, 488);
        title ("OVERSAMPLING", 858, 488);

        g.setColour (c.txt2);
        g.setFont (Fonts::sans (10.5f));
        g.drawText (abText, 640, 26, 140, 18, juce::Justification::centredRight);
        g.drawText (osText, 858, 578, 160, 16, juce::Justification::centredLeft);
    }

    void resized() override
    {
        curva.setBounds (26, 132, 159, 36);
        techo.setBounds (14, 184, 184, 172);
        densidad.setBounds (14, 364, 184, 136);
        entrada.setBounds (22, 518, 82, 104);
        salida.setBounds (108, 518, 82, 104);

        prevBtn.setBounds (282, 23, 24, 24);
        nextBtn.setBounds (310, 23, 24, 24);
        presetBtn.setBounds (342, 23, 290, 24);
        undoBtn.setBounds (786, 23, 24, 24);
        redoBtn.setBounds (814, 23, 24, 24);
        aBtn.setBounds (842, 23, 24, 24);
        bBtn.setBounds (870, 23, 24, 24);
        unifyBtn.setBounds (898, 23, 92, 24);
        gearBtn.setBounds (996, 23, 24, 24);

        display.setBounds (kGraph);
        advBtn.setBounds (kGraph.getRight() - 12 - 80, kGraph.getY() + 12, 80, 24);
        metersBtn.setBounds (advBtn.getX() - 8 - 84, kGraph.getY() + 12, 84, 24);
        curveBtn.setBounds (metersBtn.getX() - 8 - 80, kGraph.getY() + 12, 80, 24);
        sticker.setBounds (200, 46, 124, 124);

        const juce::Rectangle<int> panel (640, 108, 374, 318);
        meters.setBounds (panel);
        curvePanel.setBounds (panel);
        advanced.setBounds (panel);
        appearance.setBounds (250, 92, 600, 470);

        procesada.setBounds (222, 514, 160, 48);
        enlace.setBounds (392, 528, 58, 28);
        original.setBounds (460, 514, 154, 48);
        smooth.setBounds (686, 506, 100, 112);
        oversampling.setBounds (858, 526, 154, 30);
    }

    void tick()
    {
        const float ceilDb = s.getRawParameterValue (id::techo)->load();
        display.tick (ceilDb);
        if (meters.isVisible()) meters.tick();
        if (curvePanel.isVisible()) curvePanel.tick();
        aBtn.setToggleState (! proc.abLinked() && proc.abActive() == 'A', juce::dontSendNotification);
        bBtn.setToggleState (! proc.abLinked() && proc.abActive() == 'B', juce::dontSendNotification);
        undoBtn.setEnabled (proc.undo.canUndo());
        redoBtn.setEnabled (proc.undo.canRedo());
        presetBtn.setButtonText (proc.presets.currentName() + es ("   ⌄"));
        curveBtn.setToggleState (curvePanel.isVisible(), juce::dontSendNotification);
        metersBtn.setToggleState (meters.isVisible(), juce::dontSendNotification);
        advBtn.setToggleState (advanced.isVisible(), juce::dontSendNotification);
        if (++slow % 10 == 0)
        {
            const auto ab = proc.abLinked() ? es ("A/B enlazados") : es ("Editando ") + juce::String::charToString (proc.abActive());
            static const char* off[] = { "Igual", "1×", "2×", "4×", "8×" };
            const int o = juce::jlimit (0, 4, (int) s.getRawParameterValue (id::calidadOffline)->load());
            const auto os = es ("CPU ") + juce::String (proc.cpuLoadPercent(), 1).replaceCharacter ('.', ',') + es (" % · Export ") + es (off[o]);
            if (ab != abText || os != osText) { abText = ab; osText = os; repaint (640, 20, 150, 30); repaint (850, 572, 170, 26); }
        }
    }

    // ---- paneles
    void showPanel (const juce::String& name)
    {
        for (Overlay* o : std::initializer_list<Overlay*> { &meters, &curvePanel, &advanced })
            o->setVisible (o == &meters ? name == "Medidores" : (o == &curvePanel ? name == es ("Curva E/S") : name == "Avanzado"));
        if (name == "Apariencia") { appearance.setVisible (true); appearance.toFront (true); appearance.open(); }
        else appearance.setVisible (false);
        if (curvePanel.isVisible()) curvePanel.tick();
        sticker.toFront (false);
        for (Overlay* o : std::initializer_list<Overlay*> { &meters, &curvePanel, &advanced, &appearance }) if (o->isVisible()) o->toFront (false);
        tick();
    }

    bool closeTopPanel()
    {
        if (appearance.isVisible()) { appearance.cancel(); return true; }
        if (meters.isVisible()) { meters.setVisible (false); metersBtn.grabKeyboardFocus(); tick(); return true; }
        if (curvePanel.isVisible()) { curvePanel.setVisible (false); curveBtn.grabKeyboardFocus(); tick(); return true; }
        if (advanced.isVisible()) { advanced.setVisible (false); advBtn.grabKeyboardFocus(); tick(); return true; }
        return false;
    }

    // ---- sticker (imagen clicable)
    struct Sticker : public juce::Component, public juce::SettableTooltipClient
    {
        juce::Image image;
        std::function<void()> onClick;
        void paint (juce::Graphics& g) override { g.drawImage (image, getLocalBounds().toFloat(), juce::RectanglePlacement::centred); }
        void mouseUp (const juce::MouseEvent& e) override { if (e.mouseWasClicked() && onClick) onClick(); }
    };

private:
    Clip5Editor& ed;
    Clip5Processor& proc;
    juce::AudioProcessorValueTreeState& s;
    CurveSelector curva;
    Knob techo, densidad, entrada, salida, smooth;
    HFader procesada, original;
    PillToggle enlace;
    Segmented oversampling;
    HistoryDisplay display;
    MetersPanel meters;
    CurvePanel curvePanel;
    AdvancedPanel advanced;
    AppearancePanel appearance;
    FlatButton prevBtn, nextBtn, presetBtn, undoBtn, redoBtn, aBtn, bBtn, unifyBtn, gearBtn, curveBtn, metersBtn, advBtn;
    Sticker sticker;
    juce::String abText, osText;
    int slow = 9; // la primera llamada ya actualiza los textos

    bool linked() const { return s.getRawParameterValue (id::enlaceMezcla)->load() > 0.5f; }

    static float complement (float db)
    {
        // potencia constante: a² + b² = 1 (ambos ≤ 0 dB)
        if (db <= params::kFaderMinDb + 0.05f) return 0.0f;
        const float a = std::min (1.0f, juce::Decibels::decibelsToGain (db));
        const float b = std::sqrt (std::max (0.0f, 1.0f - a * a));
        return b < 0.001f ? params::kFaderMinDb : std::max (params::kFaderMinDb, juce::Decibels::gainToDecibels (b));
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
        for (int i = 0; i < pm.numFactory(); ++i) m.addItem (1 + i, pm.factoryName (i), true, pm.currentName() == pm.factoryName (i));
        const auto user = pm.userPresets();
        if (! user.isEmpty())
        {
            m.addSectionHeader (es ("Usuario"));
            for (int i = 0; i < user.size(); ++i) m.addItem (100 + i, user[i].getFileNameWithoutExtension(), true, pm.currentName() == user[i].getFileNameWithoutExtension());
        }
        m.addSeparator();
        m.addItem (900, es ("Guardar preset…"));
        m.addItem (901, es ("Abrir carpeta de presets"));
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [this, user] (int r) {
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
        m.addItem (500, es ("Apariencia…  (tema: ") + pal().nombre + ")");
        m.addSubMenu (es ("Escala de la interfaz"), sc);
        m.addSubMenu (es ("Frecuencia del gráfico"), fr);
        m.addSeparator();
        m.addItem (2000, es ("clip5 ") + CLIP5_VERSION_STRING + es (" · latencia ") + juce::String (proc.getLatencySamples()) + es (" muestras"), false, false);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&gearBtn), [this] (int r) {
            if (r == 500) ed.showPanel ("Apariencia");
            else if (r >= 75 && r <= 200) ed.setScale ((float) r / 100.0f);
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
    content->setFocusContainerType (juce::Component::FocusContainerType::keyboardFocusContainer);
    addMouseListener (this, true);
    setWantsKeyboardFocus (true);
    setResizable (true, true);
    setResizeLimits ((int) (kW * 0.75f), (int) (kH * 0.75f), kW * 2, kH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kW / kH);
    setSize ((int) (kW * scale), (int) (kH * scale));
    Themes::get().addChangeListener (this);
    content->tick();
    restartTimer();
}

Clip5Editor::~Clip5Editor()
{
    Themes::get().removeChangeListener (this);
    stopTimer();
    tooltips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void Clip5Editor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    lnf.applyPalette();
    content->updateSticker();
    repaint();
    content->repaint();
}

void Clip5Editor::paint (juce::Graphics& g) { g.fillAll (pal().fondo); }

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
void Clip5Editor::showPanel (const juce::String& name) { content->showPanel (name); }

void Clip5Editor::mouseDown (const juce::MouseEvent&) { proc.undo.beginNewTransaction(); }

bool Clip5Editor::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::escapeKey) return content->closeTopPanel();
    if (k == juce::KeyPress ('z', juce::ModifierKeys::commandModifier, 0)) { proc.undo.undo(); return true; }
    if (k == juce::KeyPress ('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0)) { proc.undo.redo(); return true; }
    return false;
}
} // namespace clip5
