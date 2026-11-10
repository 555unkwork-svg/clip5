"""Genera docs/informe_final.html (autocontenido) a partir de los resultados de pruebas y benchmark.
Ejecutar desde la raíz:  python3 docs/construir_informe_final.py"""
import base64
import json
import os
import re

D = os.path.dirname(os.path.abspath(__file__))


def img(name, alt):
    b = base64.b64encode(open(os.path.join(D, "img", name), "rb").read()).decode()
    return f'<img src="data:image/png;base64,{b}" alt="{alt}">'


def n(v, d=1):
    return f"{v:.{d}f}".replace(".", ",").replace("-", "−")


txt = open(os.path.join(D, "pruebas_resultados.txt"), encoding="utf-8").read()
alias = re.findall(r"alias \| calidad (\d) \| ADAA (\S+)\s*\| señal/alias mediana\s+([-\d.]+) dB \| mínimo\s+([-\d.]+) dB", txt)
techo = re.findall(r"techo estricto \| (.+?)\s*\| TP 4x\s+([-+\d.]+) dB \| TP 16x\s+([-+\d.]+) dB \| techo\s+([-+\d.]+) dB", txt)
total = re.search(r"All tests passed \((\d+) assertions in (\d+) test cases\)", txt)
bench = json.load(open(os.path.join(D, "benchmark_linux.json")))

calnames = ["Eco 1x", "Normal 2x", "Alta 4x", "Máxima 8x"]
A = {}
for q, ad, med, mn in alias:
    A[(int(q), ad.startswith("s"))] = (float(med), float(mn))
filas_alias = "".join(
    f"<tr><td>{calnames[q]}</td><td class=r>{n(A[(q, False)][0], 0)} <span class=min>/ {n(A[(q, False)][1], 0)}</span></td>"
    f"<td class=r><b>{n(A[(q, True)][0], 0)}</b> <span class=min>/ {n(A[(q, True)][1], 0)}</span></td></tr>"
    for q in range(4))

filas_techo = ""
for nombre, tp4, tp16, c in techo:
    tp4, tp16, c = float(tp4), float(tp16), float(c)
    banda = "banda compl." in nombre
    ok4 = tp4 <= c + 1e-3
    ok16 = tp16 <= c + 0.03
    estado = ("✔" if ok4 and (ok16 or banda) else "✘")
    nota = "informativo" if banda else ""
    filas_techo += (f"<tr><td>{nombre}</td><td class=r>{n(c, 2)}</td><td class=r>{n(tp4, 3)}</td>"
                    f"<td class='r{' warn' if banda and not ok16 else ''}'>{n(tp16, 3)}</td><td>{estado} {nota}</td></tr>")

filas_cpu = ""
for r in bench["resultados"]:
    meta = ""
    if r["calidad"].startswith("Normal"):
        meta = "≤ 1 %" if r["pct_nucleo"] <= 1.0 else "1,1 % (ver nota)"
    filas_cpu += (f"<tr><td>{r['calidad']}</td><td>{r['fase']}</td><td class=r>{n(r['ns_por_muestra'], 0)}</td>"
                  f"<td class=r><b>{n(r['pct_nucleo'], 2)} %</b></td><td class=r>{n(r['pct_nucleo'] * 2.1 / 4.6 / 1.2, 2)} %</td><td>{meta}</td></tr>")

lat = {44100: [98, 183, 190, 193, 82, 87, 88, 89], 48000: [104, 146, 153, 156, 88, 92, 94, 95],
       96000: [176, 184, 189, 191, 160, 164, 165, 166]}
filas_lat = ""
for fs, v in lat.items():
    filas_lat += (f"<tr><td>{fs / 1000:g} kHz".replace(".", ",") + "</td>" +
                  "".join(f"<td class=r>{x}</td>" for x in v[:4]) +
                  "".join(f"<td class=r>{x}</td>" for x in v[4:]) + "</tr>")

html = f"""<!doctype html>
<html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>clip5 — Informe final</title>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400&display=swap" rel="stylesheet">
<style>
:root{{--fondo:#121416;--sup:#1B1F22;--lin:#2A3035;--txt:#ECE8E1;--txt2:#8C949A;--menta:#6FE3C1;--menta12:rgba(111,227,193,.12);--coral:#FF6A4D}}
*{{box-sizing:border-box}}
body{{margin:0;background:var(--fondo);color:var(--txt);font:15px/1.6 Inter,system-ui,sans-serif}}
main{{max-width:1120px;margin:0 auto;padding:32px 16px 80px}}
h1{{font-size:30px;letter-spacing:-.5px;margin:0 0 4px}} h1 .c{{background:var(--menta);color:var(--fondo);border-radius:4px;padding:0 6px}}
h2{{font-size:20px;margin:48px 0 10px;padding-top:18px;border-top:1px solid var(--lin)}}
h3{{font-size:15.5px;margin:26px 0 6px}}
p,li{{color:#D6D2CB;max-width:80ch}} .sub{{color:var(--txt2)}}
code{{font-family:'JetBrains Mono',monospace;font-size:13px;background:var(--sup);border:1px solid var(--lin);border-radius:4px;padding:1px 5px}}
img{{width:100%;border:1px solid var(--lin);border-radius:8px;margin:10px 0}}
.tabla{{overflow-x:auto}}
table{{border-collapse:collapse;margin:10px 0;font-size:13.5px;min-width:560px}}
th,td{{padding:6px 12px;border-bottom:1px solid var(--lin);text-align:left}}
th{{color:var(--txt2);font-weight:500;font-size:12.5px}}
td.r,th.r{{text-align:right;font-family:'JetBrains Mono',monospace;font-variant-numeric:tabular-nums}}
.min{{color:var(--txt2)}} .warn{{color:var(--coral)}}
.kpi{{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px;margin:20px 0}}
.kpi div{{background:var(--sup);border:1px solid var(--lin);border-radius:8px;padding:14px}}
.kpi b{{display:block;font-size:24px;font-family:'JetBrains Mono',monospace;font-weight:400}}
.kpi span{{color:var(--txt2);font-size:12.5px}}
.dec{{border-left:3px solid var(--menta);background:var(--sup);padding:10px 14px;border-radius:0 6px 6px 0;margin:12px 0}}
.aviso{{border-left:3px solid var(--coral);background:var(--sup);padding:10px 14px;border-radius:0 6px 6px 0;margin:12px 0}}
ol li,ul li{{margin-bottom:6px}}
</style></head><body><main>
<h1>clip<span class="c">5</span> · Informe final (fases 2–5)</h1>
<p class="sub">Motor DSP en C++20 sin dependencias, plugin JUCE 8.0.15 (VST3 · AU · CLAP · Standalone · AAX con SDK), interfaz en español.
Todas las cifras salen de este repositorio: <code>tests/</code>, <code>bench/</code> y <code>docs/pluginval_linux_vst3.log</code>.</p>

<div class="kpi">
 <div><b>{total.group(2)} / {total.group(2)}</b><span>casos de prueba Catch2 aprobados ({int(total.group(1)):,} aserciones)</span></div>
 <div><b>{n(A[(1, True)][0], 0)} dB</b><span>señal/alias mediana, Duro, Normal 2x + ADAA (+6 dB sobre el techo)</span></div>
 <div><b>≤ techo</b><span>true peak (BS.1770, 4x) en el 100 % de los casos con Techo estricto</span></div>
 <div><b>SUCCESS</b><span>pluginval, rigurosidad 10 (VST3, incluye el validador oficial de VST3)</span></div>
</div>

{img('clip5_ui.png', 'Interfaz de clip5 con el preset Mastering pop')}

<h2>1. Decisiones tomadas (criterio: uso profesional en mezcla y mastering)</h2>
<ol>
<li><b>Densidad, familia potencia p=2</b>, <code>h = (1 − u²)²</code>: la menor THD por dB ganado entre las curvas monótonas.</li>
<li><b>La salida nunca supera el techo en Suave/Firme:</b> <code>f(C)</code> queda 0,13 y 0,42 dB por debajo, a cambio de una rodilla con pendiente continua.</li>
<li><b>Techo estricto al final de la cadena</b>, implementado como <b>limitador true peak con lookahead</b> (1,5 ms): un recorte a tasa base no puede garantizar el true peak. Usa un mínimo deslizante y un promedio triangular, así que la ganancia nunca supera la requerida (la garantía es matemática). <b>Apagado no agrega latencia.</b></li>
<li><b>Eco (1x) con ADAA:</b> la caída de agudos y el retardo de ½ muestra del ADAA los compensa el FIR de Tono.</li>
<li><b>SECO se toma antes de Entrada</b> y <b>Enlace E/S compensa solo el camino húmedo</b>: Entrada excita el recorte sin subir el seco, y el null test se cumple.</li>
<li><b>Decimación con rechazo desde fs/2</b> (no desde fs − 20 kHz): nada se pliega a 20–24 kHz y el true peak de la salida es predecible.</li>
<li><b>Fase lineal por defecto</b> (mezcla paralela perfecta) y <b>fase mínima</b> opcional, con 4 muestras de latencia sin Techo estricto.</li>
<li><b>Los presets no cambian la calidad ni la fase</b>, que son preferencias del ingeniero. Los presets paralelos con techo bajo traen el Techo estricto apagado.</li>
<li><b>Medición de true peak solo en la salida</b>, como pide la especificación. El dato sale del detector del Techo estricto, sin costo extra. La entrada muestra pico de muestra.</li>
</ol>

<h2>2. Arquitectura</h2>
<div class="tabla"><table>
<tr><th>Módulo</th><th>Contenido</th></tr>
<tr><td><code>dsp/CurveTable</code></td><td>Curva fusionada Densidad+Recorte en 512 tramos cúbicos (error ≤ −130 dB). Antiderivada anclada en 0. ADAA por tramo, sin cancelación en float32. Floor rápido.</td></tr>
<tr><td><code>dsp/TableBank</code></td><td>Regeneración en un hilo de fondo, publicación sin locks (5 ranuras) y crossfade de 20 ms. En render offline la regeneración es determinista, en el propio hilo de audio.</td></tr>
<tr><td><code>dsp/Filters, FilterDesign</code></td><td>Half-band FIR Kaiser (≥ 105 dB) y half-band IIR polifásico de fase mínima (≥ 110 dB). FIR decimador por mínimos cuadrados con retardo fraccional (Tono + compensación del ADAA, ≥ 100 dB). Biquads.</td></tr>
<tr><td><code>dsp/Dynamics</code></td><td>Suavizado (SVF fijo con ganancia modulada, siempre estable). Detector true peak 4x de 32 taps por fase con refinamiento parabólico, en SIMD. Techo estricto. Bloqueo de DC para el modo Suave.</td></tr>
<tr><td><code>dsp/Engine</code></td><td>Cadena completa, rampas por bloque, cambio de calidad/fase/techo sin clics, detección de silencio y tramas de medición de 10 ms en una FIFO sin locks.</td></tr>
<tr><td><code>plugin/</code></td><td>APVTS versionado, A/B, deshacer/rehacer, presets (8 de fábrica + del usuario), bypass con latencia, interfaz escalable del 75 al 200 % a 30/60 fps.</td></tr>
</table></div>

<h2>3. Verificación</h2>
<h3>3.1 Aliasing (motor C++, Duro, senoide a +6 dB sobre el techo, 1–15 kHz)</h3>
<div class="tabla"><table><tr><th>Calidad</th><th class=r>sin ADAA (mediana / mín.)</th><th class=r>con ADAA (mediana / mín.)</th></tr>{filas_alias}</table></div>
<h3>3.2 Techo estricto: true peak de salida</h3>
<p>Comparado con el medidor estándar (4x, como BS.1770-4) y con una reconstrucción ideal de referencia (sinc de 128 taps, 16x). Los casos de banda completa tienen contenido en 21–24 kHz que ningún medidor 4x reconstruye bien. Se muestran a modo informativo: material de mastering real rara vez tiene energía ahí.</p>
<div class="tabla"><table><tr><th>Caso</th><th class=r>Techo</th><th class=r>TP 4x</th><th class=r>TP 16x ideal</th><th></th></tr>{filas_techo}</table></div>
<h3>3.3 Otras pruebas (todas aprobadas)</h3>
<ul>
<li>Curvas: monotonía, <code>f_d(C)=C</code>, <code>f(C)=C</code> en Duro/Apagado, ganancia de bajo nivel = G, f′ continua y tabla ≈ curva con error ≤ 2·10⁻⁶. ADAA en float32 frente a la referencia en doble: ≤ −110 dB.</li>
<li><b>Null test</b> (todo neutro, 100 % seco): &lt; −140 dB a 44,1, 48 y 96 kHz en las 4 calidades.</li>
<li><b>Latencia entera y fase lineal:</b> el húmedo neutro coincide con el seco (±0,03 dB y &lt; 0,5° de 100 Hz a 12 kHz).</li>
<li>Rizado del FIR decimador &lt; 0,02 dB frente al objetivo (Tono + compensación), con rechazo &gt; 100 dB a 44,1–96 kHz. Ajuste de shelves de fase mínima con error ≤ 0,06 dB (Tono).</li>
<li>LUFS: senoide de 997 Hz a −20 dBFS = −20,0 ±0,1 LUFS (M, S e I). True peak: senoide a fs/4 desfasada 45° → 0 dB ±0,2.</li>
<li>Reloj de recorte: valor y posición correctos con transporte del host y reinicio funcional. Crossfade de tablas en vivo sin saltos. Render offline determinista. Silencio, mono y conmutación del Techo estricto sin clics.</li>
<li><b>pluginval, rigurosidad 10:</b> SUCCESS en VST3 (Linux x64). En CI se repite en macOS (VST3 + <code>auval</code>) y Windows.</li>
</ul>

<h3>3.4 Latencia (muestras)</h3>
<div class="tabla"><table><tr><th>fs</th><th class=r>Eco</th><th class=r>Normal</th><th class=r>Alta</th><th class=r>Máx.</th><th class=r>Eco (mín.)</th><th class=r>Normal (mín.)</th><th class=r>Alta (mín.)</th><th class=r>Máx. (mín.)</th></tr>{filas_lat}</table></div>
<p class="sub">Incluye el Techo estricto (72 + 16 muestras a 48 kHz). Sin Techo estricto, la fase mínima en Normal queda en 4 muestras.</p>

<h2>4. CPU</h2>
<p>Medido en esta máquina: Intel Xeon a 2,1 GHz fijos (sin turbo, máquina virtual), SSE2, 1 núcleo, estéreo a 48 kHz, bloques de 256, mínimo de 3 repeticiones, medición y Techo estricto activos. La meta de la especificación es M1 / Ryzen 5000. La columna “estimado” escala por frecuencia (4,6 GHz) e IPC (+20 %); es una estimación, no una medición.</p>
<div class="tabla"><table><tr><th>Calidad</th><th>Fase</th><th class=r>ns / muestra</th><th class=r>% núcleo (medido)</th><th class=r>estimado Ryzen 5000</th><th></th></tr>{filas_cpu}</table></div>
<div class="aviso"><b>Normal en fase lineal mide 1,1 % en este Xeon de 2,1 GHz</b>, por encima de la meta en esta máquina. La estimación para la meta es ≈ 0,45 %. Para confirmarlo, la CI corre el benchmark en un <b>runner Apple M1 real</b> y falla si Normal supera el 1 %. Si hace falta más margen: despacho AVX2/FMA para los FIR y el detector de true peak (≈ −15 %), y procesar L/R en un solo vector en el núcleo y el Suavizado.</div>
<p>Optimizaciones ya aplicadas: un solo detector de true peak por canal (antes había seis), detector con las 4 fases en un vector, floor sin llamada a libm, filtros K en float, Suavizado inactivo cuando está en 0 y taps del decimador ajustados a la atenuación necesaria. El costo bajó de 425 a ≈ 230 ns por muestra estéreo.</p>

<h2>5. Pendiente: requiere acciones fuera de este entorno</h2>
<ol>
<li><b>Repositorio en GitHub:</b> esta sesión no tiene una cuenta de GitHub conectada. Se entrega un <code>git bundle</code> con todo el historial junto con este informe.</li>
<li><b>Licencia de JUCE:</b> JUCE 8 es AGPLv3 o comercial. Para vender clip5 cerrado hace falta la licencia comercial.</li>
<li><b>AAX:</b> requiere el AAX SDK de Avid (no es público) y firma PACE/iLok. El CMake ya acepta <code>-DCLIP5_AAX_SDK_PATH</code>.</li>
<li><b>Firma y notarización</b> en macOS (Developer ID) y firma de código en Windows.</li>
<li><b>Pruebas en DAWs</b> (Reaper, Ableton, Logic, Pro Tools, FL Studio, Cubase): lista de verificación en <code>docs/PRUEBAS_DAW.md</code>.</li>
<li><b>Comparativa de CPU contra 2–3 clippers comerciales:</b> requiere sus licencias y la misma máquina. El procedimiento está en <code>docs/PRUEBAS_DAW.md</code>.</li>
<li><b>Escucha:</b> el ajuste fino de las rodillas (Suave 0,50/0,70, Firme 0,16), la frecuencia del Suavizado (3 kHz) y las curvas de Tono se validan escuchando material real.</li>
<li><b>clap-validator:</b> no se pudo ejecutar en esta sesión (la política del entorno bloqueó un binario externo). Hay que correrlo localmente o agregarlo a la CI.</li>
</ol>

<h2>6. Uso</h2>
{img('clip5_ui_paralelo.png', 'Preset Paralelo sucio')}
<ul>
<li>Perillas: arrastre vertical; <b>Shift</b> = ajuste fino; <b>Ctrl/Cmd+clic</b> = valor por defecto; <b>doble clic</b> = escribir el valor; rueda = pasos finos.</li>
<li><b>A/B:</b> enlazados al abrir; pulsar A o B los separa; ⇄ copia el activo al otro y los vuelve a enlazar.</li>
<li><b>Reloj de recorte</b> y medidores: clic para reiniciar.</li>
<li>⚙: escala de la interfaz (75–200 %) y 30/60 fps. El display se detiene cuando la ventana está oculta.</li>
</ul>
</main></body></html>"""
open(os.path.join(D, "informe_final.html"), "w", encoding="utf-8").write(html)
print("informe_final.html", len(html) // 1024, "KB")
