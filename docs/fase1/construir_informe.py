"""Genera docs/fase1/informe.html (autocontenido: figuras embebidas en base64).
Ejecutar después de proto/analisis.py:  python3 docs/fase1/construir_informe.py"""
import base64
import json
import math
import os

AQUI = os.path.dirname(os.path.abspath(__file__))
R = json.load(open(os.path.join(AQUI, "resultados.json")))


def img(nombre, alt):
    b = base64.b64encode(open(os.path.join(AQUI, "figuras", nombre), "rb").read()).decode()
    return f'<img src="data:image/png;base64,{b}" alt="{alt}">'


def n(v, d=1):
    s = f"{v:.{d}f}".replace(".", ",")
    return s.replace("-", "−")


# ---------------------------------------------------------------- tablas
fam = sorted(R["densidad_familias"].items(), key=lambda kv: kv[1]["puntaje"])
filas_fam = "".join(
    f'<tr class="{"gana" if k == R["densidad_ganadora"] else ""}{" off" if not v["monotona"] else ""}">'
    f'<td>{k}</td><td>{"sí" if v["monotona"] else "no"}</td><td class="r">{n(v["pendiente_min"], 2)}</td>'
    f'<td class="r">{n(v["thd_pct_medio"], 2)}</td><td class="r">{n(v["ganancia_rms_media_db"], 2)}</td>'
    f'<td class="r">{n(v["puntaje"], 3)}</td></tr>' for k, v in fam)

al = R["aliasing"]["sar_db"]
cal = ["Eco 1x", "Normal 2x", "Alta 4x", "Máxima 8x"]


def med(v):
    import statistics
    return statistics.median(v)


filas_al = ""
for c in cal:
    filas_al += f"<tr><td>{c}</td>"
    for modo in ("Duro", "Firme"):
        for ad in ("", " + ADAA"):
            v = al[modo][c + ad]
            filas_al += f'<td class="r">{n(med(v), 0)} <span class="min">/ {n(min(v), 0)}</span></td>'
    filas_al += "</tr>"

caida = {c: 20 * math.log10(math.cos(math.pi * 20000 / (48000 * f)))
         for c, f in zip(cal, (1, 2, 4, 8))}
filas_caida = "".join(f'<tr><td>{c}</td><td class="r">{n(v, 2)} dB</td>'
                      f'<td class="r">{n(20 * math.log10(math.cos(math.pi * 10000 / (48000 * f))), 2)} dB</td>'
                      f'<td class="r">{n(0.5 / f, 4)}</td></tr>'
                      for (c, v), f in zip(caida.items(), (1, 2, 4, 8)))

tab = R["tabla_error_max_db"]
filas_tab = "".join(f"<tr><td>{m}</td>" + "".join(f'<td class="r">{n(tab[m][k], 0)}</td>' for k in ("64", "128", "256", "512", "1024")) + "</tr>"
                    for m in tab)

pr = R["adaa_float32_error_db"]
filas_pr = "".join(f"<tr><td>{nv}</td>" + "".join(f'<td class="r">{n(v[k], 0)}</td>' for k in ("ingenuo ε=1e-5", "ingenuo ε=1e-3", "por tramo")) + "</tr>"
                   for nv, v in pr.items())

arm = R["armonicos_+3dB_sobre_techo"]
filas_arm = "".join(f"<tr><td>{m}</td>" + "".join(f'<td class="r">{n(max(v[h], -140), 0)}</td>' for h in ("H2", "H3", "H4", "H5")) + "</tr>"
                    for m, v in arm.items())

# ---------------------------------------------------------------- diagrama de la cadena (SVG)
def caja(x, y, w, h, t1, t2="", t3="", acento=False, tenue=False):
    borde = "var(--menta)" if acento else "var(--lin)"
    fondo = "var(--menta12)" if acento else "var(--sup)"
    op = ' opacity=".9"' if tenue else ""
    s = f'<g{op}><rect x="{x}" y="{y}" width="{w}" height="{h}" rx="7" fill="{fondo}" stroke="{borde}" stroke-width="1.3"/>'
    s += f'<text x="{x + w / 2}" y="{y + 20}" class="t1">{t1}</text>'
    if t2:
        s += f'<text x="{x + w / 2}" y="{y + 37}" class="t2">{t2}</text>'
    if t3:
        s += f'<text x="{x + w / 2}" y="{y + 52}" class="t2">{t3}</text>'
    return s + "</g>"


def flecha(x1, y1, x2, y2):
    return f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" class="fl" marker-end="url(#pt)"/>'


Y, H = 40, 66
bloques = [
    (10, 92, "1 · Entrada", "trim ±12 dB", "enlace E/S"),
    (118, 74, "2 · ↑N", "half-band", "polifásico"),
    (208, 150, "3 · Núcleo fusionado", "Densidad + Recorte", "tabla 512 tramos + ADAA"),
    (374, 116, "4 · Suavizado", "high-shelf SVF", "detector = |u| (gratis)"),
    (506, 136, "5 · ↓N + Tono", "FIR polifásico único", "+ comp. ADAA"),
    (658, 104, "6 · Techo", "estricto", "ADAA, tasa base"),
    (778, 96, "7 · Ganancia", "unitaria (−G)", "→ escalar"),
]
svg = ['<svg viewBox="0 0 1100 232" class="cadena" role="img" aria-label="Diagrama de la cadena de señal de clip5">',
       '<defs><marker id="pt" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto">'
       '<path d="M0 0L10 5L0 10z" fill="#8C949A"/></marker></defs>']
svg.append('<rect x="112" y="24" width="538" height="100" rx="10" fill="none" stroke="#6FE3C1" stroke-dasharray="4 4" stroke-opacity=".55"/>')
svg.append('<text x="120" y="18" class="t3 izq">tasa sobremuestreada N·fs (N = 1, 2, 4, 8 según Calidad)</text>')
for i, (x, w, a, b, c) in enumerate(bloques):
    svg.append(caja(x, Y, w, H, a, b, c, acento=(i in (2, 4))))
    if i:
        px, pw = bloques[i - 1][0], bloques[i - 1][1]
        svg.append(flecha(px + pw, Y + H / 2, x - 2, Y + H / 2))
svg.append(flecha(874, Y + H / 2, 900, Y + H / 2))
svg.append(caja(902, Y, 92, H, "8 · Mezcla", "HÚMEDO / SECO", "paralelo real"))
svg.append(flecha(994, Y + H / 2, 1010, Y + H / 2))
svg.append(caja(1012, Y, 84, H, "9 · Salida", "trim ±12 dB", "enlace E/S"))
# rama seca
svg.append(f'<path d="M102 {Y + H / 2} L106 {Y + H / 2} L106 196 L470 196" class="fl" fill="none"/>')
svg.append(caja(472, 172, 160, 48, "Línea de retardo", "= latencia exacta del húmedo"))
svg.append(f'<path d="M632 196 L948 196 L948 {Y + H + 2}" class="fl" fill="none" marker-end="url(#pt)"/>')
svg.append('<text x="300" y="190" class="t3">SECO (tomado después de Entrada)</text>')
# medición
svg.append('<text x="283" y="140" class="t3 menta">medidores Recorte y Densidad (del núcleo)</text>')
svg.append('<text x="1044" y="140" class="t3">medición salida:</text>')
svg.append('<text x="1044" y="154" class="t3">pico · TP 4x · LUFS</text>')
svg.append('<text x="56" y="140" class="t3">medición entrada</text>')
svg.append("</svg>")
SVG = "\n".join(svg)

# ---------------------------------------------------------------- HTML
html = f"""<!doctype html>
<html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>clip5 — Fase 1</title>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400&display=swap" rel="stylesheet">
<style>
:root{{--fondo:#121416;--sup:#1B1F22;--lin:#2A3035;--txt:#ECE8E1;--txt2:#8C949A;--menta:#6FE3C1;--menta12:rgba(111,227,193,.12);--coral:#FF6A4D}}
*{{box-sizing:border-box}}
body{{margin:0;background:var(--fondo);color:var(--txt);font:15px/1.6 Inter,system-ui,sans-serif}}
main{{max-width:1120px;margin:0 auto;padding:32px 16px 80px}}
h1{{font-size:30px;letter-spacing:-.5px;margin:0 0 4px}} h1 .c{{background:var(--menta);color:var(--fondo);border-radius:4px;padding:0 6px}}
h2{{font-size:20px;margin:48px 0 10px;padding-top:18px;border-top:1px solid var(--lin)}}
h3{{font-size:15.5px;margin:26px 0 6px}}
p,li{{color:#D6D2CB;max-width:78ch}} .sub{{color:var(--txt2)}}
code,.num{{font-family:'JetBrains Mono',monospace;font-size:13px;font-variant-numeric:tabular-nums}}
code{{background:var(--sup);border:1px solid var(--lin);border-radius:4px;padding:1px 5px}}
img{{width:100%;border:1px solid var(--lin);border-radius:8px;margin:10px 0}}
.tabla{{overflow-x:auto}}
table{{border-collapse:collapse;margin:10px 0;font-size:13.5px;min-width:520px}}
th,td{{padding:6px 12px;border-bottom:1px solid var(--lin);text-align:left}}
th{{color:var(--txt2);font-weight:500;font-size:12.5px}}
td.r,th.r{{text-align:right;font-family:'JetBrains Mono',monospace;font-variant-numeric:tabular-nums}}
tr.gana td{{color:var(--menta)}} tr.off td{{color:var(--txt2)}}
.min{{color:var(--txt2)}}
.kpi{{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px;margin:20px 0}}
.kpi div{{background:var(--sup);border:1px solid var(--lin);border-radius:8px;padding:14px}}
.kpi b{{display:block;font-size:24px;font-family:'JetBrains Mono',monospace;font-weight:400;color:var(--txt)}}
.kpi span{{color:var(--txt2);font-size:12.5px}}
.aviso{{border-left:3px solid var(--coral);background:var(--sup);padding:10px 14px;border-radius:0 6px 6px 0;margin:14px 0}}
.dec{{border-left:3px solid var(--menta);background:var(--sup);padding:10px 14px;border-radius:0 6px 6px 0;margin:14px 0}}
.cadena{{width:100%;min-width:900px;background:var(--sup);border:1px solid var(--lin);border-radius:8px}}
.cadena .t1{{fill:var(--txt);font:600 12px Inter;text-anchor:middle}}
.cadena .t2{{fill:var(--txt2);font:11px Inter;text-anchor:middle}}
.cadena .t3{{fill:var(--txt2);font:11px Inter;text-anchor:middle}} .cadena .izq{{text-anchor:start}} .cadena .menta{{fill:var(--menta)}}
.cadena .fl{{stroke:#8C949A;stroke-width:1.3}}
ol.q li{{margin-bottom:10px}}
</style></head><body><main>
<h1>clip<span class="c">5</span> · Fase 1</h1>
<p class="sub">Cadena de señal, prototipo de curvas en Python/numpy, análisis de aliasing y mockup de interfaz. Todo el código está en <code>proto/</code>; las cifras salen de <code>docs/fase1/resultados.json</code>, generado por <code>proto/analisis.py</code>.</p>

<div class="kpi">
 <div><b>{n(med(al['Duro']['Normal 2x + ADAA']), 0)} dB</b><span>señal/alias mediana, Duro, Normal 2x + ADAA (+6 dB sobre el techo, 1–20 kHz)</span></div>
 <div><b>+{n(med(al['Duro']['Normal 2x + ADAA']) - med(al['Duro']['Normal 2x']), 0)} dB</b><span>mejora del ADAA sobre 2x sin ADAA (Duro)</span></div>
 <div><b>{n(pr['-40 dB re techo']['por tramo'], 0)} dB</b><span>error del ADAA en float32 a −40 dB (variante por tramo, tabla anclada en 0)</span></div>
 <div><b>84 / 84</b><span>verificaciones de curvas OK (monotonía, f(C), ganancia G, f′ continua, F1)</span></div>
</div>

<h2>1. Cadena de señal</h2>
<div class="tabla">{SVG}</div>
<p>Orden definitivo según la especificación. Decisiones de esta fase:</p>
<ul>
<li><b>Todo se normaliza al techo</b> (<code>u = x/C</code>, salida <code>C·f(u)</code>). La tabla del núcleo solo depende de Densidad y Modo; mover el Techo no regenera nada (es un escalar antes y otro después).</li>
<li><b>SECO se toma después de Entrada</b>: así el Enlace E/S (+1 dB entrada / −1 dB salida) no cambia el nivel del seco y el null test con todo neutro sigue siendo exacto.</li>
<li><b>El FIR de decimación + Tono absorbe dos efectos del ADAA</b>: su retardo de ½ muestra (a tasa N·fs) y su caída de agudos (ver §4.3). Al diseñar ese FIR con un retardo de grupo de k + ½ muestras, la latencia total queda entera y la compensación del seco es una línea de retardo simple.</li>
</ul>

<h2>2. Densidad: elección de la familia</h2>
<p><code>f_d(u) = u·g(|u|)</code>, <code>g = 1 + (g₀ − 1)·h(u)</code>, con <code>h(0)=1</code>, <code>h(1)=0</code>, <code>h′(1)=0</code> (pendiente 1 en el techo, f′ continua). Criterio: menor THD promedio por dB de RMS ganado, con senoides de −24 a 0 dB re techo, G = +6 dB (el peor caso).</p>
{img('01_densidad_familias.png', 'Ganancia de Densidad por familia y THD frente a ganancia RMS')}
<div class="tabla"><table><tr><th>Familia</th><th>Monótona a +6 dB</th><th class="r">Pendiente mín.</th><th class="r">THD medio %</th><th class="r">Ganancia RMS dB</th><th class="r">%THD / dB</th></tr>{filas_fam}</table></div>
<div class="dec"><b>Ganadora: {R['densidad_ganadora']}</b>, <code>h = (1 − u²)²</code>. Barata (3 multiplicaciones, aunque igual va a tabla). La potencia p=3 puntúa mejor pero <b>no es monótona</b> a +6 dB (pendiente negativa = inversión de nivel), así que queda descartada. Alternativa con más margen: smoothstep (pendiente mínima 0,32 contra 0,20; 5 % peor en el criterio). La decisión final conviene tomarla escuchando; las dos están implementadas.</div>

<h2>3. Modos de recorte y curva fusionada</h2>
<p>Rodilla C² (la pendiente baja de 1 a 0 siguiendo un smoothstep) de ancho <i>w</i>, centrada en el techo: lineal hasta <code>1 − w/2</code> y saturación exacta en 1 desde <code>1 + w/2</code>. Suave: w = 0,50 (positivo) / 0,70 (negativo), asimetría → armónicos pares. Firme: w = 0,16 simétrico → solo impares. Duro: w = 0. Apagado: sin recorte.</p>
{img('02_curvas_modos.png', 'Curvas de transferencia fusionadas por modo')}
<div class="tabla"><table><tr><th>Armónicos (dB re fundamental), senoide a +2,9 dB sobre el techo</th><th class="r">H2</th><th class="r">H3</th><th class="r">H4</th><th class="r">H5</th></tr>{filas_arm}</table></div>
<div class="aviso"><b>Conflicto en la especificación que necesita tu decisión:</b> pide <code>f(C) = C</code> y a la vez rodilla suave sin superar el techo. Una curva cóncava con pendiente continua no puede cumplir las dos cosas. Hoy: <code>f_d(C) = C</code> en todos los modos y <code>f(C) = C</code> en Duro y Apagado; en Firme <code>f(C) = C − 0,13 dB</code> y en Suave <code>C − 0,42 dB</code>, y la salida nunca supera C. Ver pregunta 2.</div>

<h3>Tabla: tamaño contra error</h3>
<p>Tramos cúbicos de Hermite con derivadas laterales (los quiebres de Duro caen en nodos y se reproducen exactos), rango [−2, 2] y extrapolación lineal exacta fuera de él. Error máximo, dB re techo, con Densidad +6 dB:</p>
<div class="tabla"><table><tr><th>Modo</th><th class="r">64</th><th class="r">128</th><th class="r">256</th><th class="r">512</th><th class="r">1024</th></tr>{filas_tab}</table></div>
<div class="dec"><b>512 tramos</b> (error ≤ −130 dB): 512 × 4 coeficientes float = 8 KB + 2 KB de la antiderivada. Entra holgado en L1, dos tablas para el crossfade incluidas.</div>

<h2>4. Aliasing con y sin ADAA</h2>
<p>Senoide a +6 dB sobre el techo, 22 frecuencias de 1 a 20 kHz, frecuencias exactas en bin (FFT de 32768 puntos sin ventana, régimen permanente). Señal/alias = potencia de los armónicos legítimos contra componentes inarmónicas en 0–20 kHz (lo que cae entre 20 y 24 kHz es inaudible y lo quita Tono). Sobremuestreo de referencia: half-band FIR Kaiser de 110 dB en cascada (87 / 27 / 19 taps).</p>
{img('03_aliasing_sar.png', 'Relación señal/alias por calidad, con y sin ADAA')}
<div class="tabla"><table><tr><th rowspan="2">Calidad</th><th colspan="2" class="r">Duro, G = 0</th><th colspan="2" class="r">Firme, G = +3 dB</th></tr>
<tr><th class="r">sin ADAA</th><th class="r">+ ADAA</th><th class="r">sin ADAA</th><th class="r">+ ADAA</th></tr>{filas_al}</table></div>
<p class="sub">Mediana / mínimo en dB sobre 1–20 kHz. El mínimo siempre está entre 16 y 20 kHz, donde casi todos los armónicos quedan fuera de banda.</p>
{img('04_espectros_duro_7k.png', 'Espectros de ejemplo, Duro a 7 kHz')}
<h3>4.1 Lectura</h3>
<ul>
<li>El ADAA vale más que duplicar el factor: <b>2x + ADAA ≈ 4x sin ADAA</b> y <b>4x + ADAA supera a 8x sin ADAA</b> en mediana. Eso respalda la estrategia de CPU.</li>
<li>Eco (1x) es usable en graves y medios, pero a partir de unos 10 kHz el 3.er armónico se pliega dentro de la banda y ningún ADAA de 1.er orden lo evita. Es lo esperable en 1x; queda documentado en el tooltip de Calidad.</li>
</ul>
<h3>4.2 Precisión del ADAA en float32 (hallazgo importante para la fase 2)</h3>
<p>La forma clásica <code>(F1(u) − F1(u₋₁)) / (u − u₋₁)</code> sufre cancelación catastrófica en float32. Comparé tres variantes contra la referencia en float64 (Firme, G = +4 dB, 997 Hz a 96 kHz). Error RMS en dB re señal:</p>
<div class="tabla"><table><tr><th>Nivel</th><th class="r">ingenuo ε=1e-5</th><th class="r">ingenuo ε=1e-3</th><th class="r">por tramo</th></tr>{filas_pr}</table></div>
<div class="dec"><b>Diseño elegido: ADAA “por tramo” y antiderivada anclada en u = 0.</b> Si u y u₋₁ caen en el mismo tramo cúbico, la media del polinomio se calcula factorizada (<code>c₀ + c₁(t+s)/2 + c₂(t²+ts+s²)/3 + c₃(t³+t²s+ts²+s³)/4</code>), sin restas que cancelen y sin ε. En tramos vecinos se parte en el nodo y se ponderan dos medias exactas; solo con Δ ≥ 1 tramo se recurre a la diferencia de F1. Con F1(0) = 0 la antiderivada es diminuta para señales bajas. Antes de anclarla, la variante ingenua con ε=1e-5 daba <b>−28 dB</b> de error a −40 dB: una distorsión audible en pasajes suaves.</div>
<h3>4.3 Efecto lineal del ADAA (la fase 3 debe compensarlo)</h3>
<p>En la zona lineal el ADAA de 1.er orden es exactamente un promedio de 2 muestras: <code>H = cos(πf/fs_N)</code> y retardo de ½ muestra a la tasa sobremuestreada.</p>
<div class="tabla"><table><tr><th>Calidad</th><th class="r">Caída a 20 kHz</th><th class="r">a 10 kHz</th><th class="r">Retardo (muestras base)</th></tr>{filas_caida}</table></div>
<p>En 2x o más, el FIR de decimación + Tono lo compensa sin costo. En <b>Eco 1x</b> la caída es grande: el FIR corto de Tono tiene que levantar hasta unos +8 dB a 18 kHz (y cortar por encima). Estimo unos 16 taps; se valida en la fase 3.</p>

<h2>5. Presupuesto de CPU (estimación, se mide en la fase 2)</h2>
<p>Meta: ≤ 1 % de un núcleo, estéreo, 48 kHz, Normal. A ~3,5 GHz eso son ≈ 730 ciclos por muestra estéreo.</p>
<div class="tabla"><table><tr><th>Etapa (Normal 2x, por muestra base y canal)</th><th class="r">Operaciones aprox.</th></tr>
<tr><td>↑2 half-band 87 taps (fase central trivial, la otra 44 coef. simétricos)</td><td class="r">~45</td></tr>
<tr><td>Núcleo: índice + 4 coef. + media factorizada, ×2 muestras</td><td class="r">~60</td></tr>
<tr><td>Suavizado: detector + SVF high-shelf, ×2</td><td class="r">~28</td></tr>
<tr><td>↓2 + Tono: FIR polifásico ~64 taps simétrico</td><td class="r">~64</td></tr>
<tr><td>Techo estricto (ADAA de recorte duro a tasa base)</td><td class="r">~12</td></tr>
<tr><td>Medición de salida: true peak 4x (3 fases × 12 taps) + K-weighting</td><td class="r">~46</td></tr>
<tr><td><b>Total</b></td><td class="r"><b>~255</b></td></tr></table></div>
<p>Con L/R en un vector SIMD y ~1–2 operaciones vectoriales por ciclo: ≈ 130–250 ciclos por muestra estéreo, o sea <b>≈ 0,2–0,35 % de un núcleo</b>, con margen de sobra para la meta. Los mockups muestran estas cifras como placeholder hasta tener el benchmark real.</p>

<h2>6. Mockup de interfaz</h2>
<p>Archivo <code>mockup/clip5_ui.html</code>, interactivo: perillas de arco fino, selector de modo con mini-curvas reales, transferencia en la esquina del display, medidores, Reloj de recorte, faders HÚMEDO/SECO con enlace de potencia constante, A/B y panel Avanzado. Paleta y tipografía según la especificación: menta como único acento de marca, coral solo para recorte y Suavizado en menta al 55 %. La marca usa el “5” en bloque menta con una muesca.</p>

<h2>7. Preguntas para aprobar la fase 1</h2>
<ol class="q">
<li><b>Familia de Densidad:</b> ¿potencia p=2 (gana el criterio, pendiente mínima 0,20) o smoothstep (más margen de monotonía, 5 % peor)? Propongo p=2 y confirmarlo en la escucha de la fase 2.</li>
<li><b>f(C) en Suave y Firme:</b> ¿aceptamos <code>f(C) = C − 0,13 / 0,42 dB</code> con la salida siempre ≤ C (recomendado), o prefieres <code>f(C) = C</code> exacto desplazando la rodilla, con lo que la salida satura un poco por encima del techo (Firme +0,13 dB, Suave +0,42 dB)?</li>
<li><b>Posición del Techo estricto:</b> en el orden actual va antes de la mezcla y de Salida, así que SECO &gt; −∞, HÚMEDO &gt; 0 dB o Salida &gt; 0 dB pueden superar el techo. Propongo moverlo <b>al final de la cadena</b> (después de Salida) para que la garantía sea absoluta. ¿Estás de acuerdo?</li>
<li><b>Eco 1x:</b> ¿lo mantenemos con ADAA + compensación en el FIR de Tono, o Eco se queda sin ADAA y plano (menos coloración en agudos, más alias)?</li>
</ol>
</main></body></html>"""
open(os.path.join(AQUI, "informe.html"), "w").write(html)
print("informe.html", len(html) // 1024, "KB")
