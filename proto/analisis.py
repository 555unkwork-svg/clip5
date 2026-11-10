"""
clip5 — análisis de la fase 1. Genera figuras en docs/fase1/figuras y resultados.json.
Ejecutar:  python3 proto/analisis.py
"""
import json
import os
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import clip5_curvas as c

AQUI = os.path.dirname(os.path.abspath(__file__))
FIG = os.path.join(AQUI, "..", "docs", "fase1", "figuras")
os.makedirs(FIG, exist_ok=True)
R = {}

# ----------------------------------------------------------------------- estilo (marca)
BG, SUP, LIN = "#121416", "#1B1F22", "#2A3035"
TXT, TXT2 = "#ECE8E1", "#8C949A"
MENTA, CORAL = "#6FE3C1", "#FF6A4D"
SERIES = ["#3987e5", "#d95926", "#199e70", "#c98500", "#d55181", "#9085e9", "#e66767", "#008300"]
plt.rcParams.update({
    "figure.facecolor": BG, "axes.facecolor": SUP, "savefig.facecolor": BG,
    "axes.edgecolor": LIN, "axes.labelcolor": TXT2, "text.color": TXT,
    "xtick.color": TXT2, "ytick.color": TXT2, "grid.color": LIN, "grid.linewidth": 0.6,
    "axes.grid": True, "font.family": ["Inter"], "font.size": 9.5,
    "axes.titlesize": 10.5, "axes.titleweight": "semibold", "axes.titlecolor": TXT,
    "axes.titlelocation": "left", "lines.linewidth": 1.6, "legend.frameon": False,
    "legend.fontsize": 8.5, "axes.spines.top": False, "axes.spines.right": False,
})


def guardar(fig, nombre):
    fig.savefig(os.path.join(FIG, nombre), dpi=160, bbox_inches="tight")
    plt.close(fig)


def db(x):
    return 20 * np.log10(np.maximum(np.abs(x), 1e-30))


# ============================================================ 1. Familias de Densidad
def thd_estatico(f, A, n=8192):
    th = 2 * np.pi * np.arange(n) / n
    x = A * np.sin(th)
    y = f(x)
    Y = np.abs(np.fft.rfft(y)) / (n / 2)
    fund = Y[1]
    arm = np.sqrt(np.sum(Y[2:200] ** 2))
    g_rms = db(np.sqrt(np.mean(y ** 2)) / np.sqrt(np.mean(x ** 2)))
    return arm / fund, g_rms


NIVELES = np.array([-24, -18, -12, -9, -6, -4, -2, -1, 0.0])
G_FAM = 6.0
fam = {}
u = np.linspace(0, 1, 100001)
for nombre, h in c.FAMILIAS.items():
    fd = c.densidad(u, G_FAM, h)
    pend = np.gradient(fd, u)
    monot = bool(pend.min() > 0)
    thds, gains = [], []
    for L in NIVELES:
        t, g = thd_estatico(lambda x: c.densidad(x, G_FAM, h), 10 ** (L / 20))
        thds.append(t); gains.append(g)
    thds, gains = np.array(thds), np.array(gains)
    fam[nombre] = dict(monotona=monot, pendiente_min=float(pend.min()),
                       thd_pct_medio=float(100 * thds.mean()), ganancia_rms_media_db=float(gains.mean()),
                       puntaje=float(100 * thds.mean() / gains.mean()),
                       thd_pct=list(100 * thds), ganancia_db=list(gains))
validas = {k: v for k, v in fam.items() if v["monotona"]}
ganadora = min(validas, key=lambda k: validas[k]["puntaje"])
R["densidad_familias"] = fam
R["densidad_ganadora"] = ganadora
print("Familias de Densidad (G=+6 dB):")
for k, v in sorted(fam.items(), key=lambda kv: kv[1]["puntaje"]):
    print(f"  {k:26s} monót={v['monotona']!s:5s} pend.min={v['pendiente_min']:.2f} "
          f"THD={v['thd_pct_medio']:.2f}% ganancia={v['ganancia_rms_media_db']:.2f} dB "
          f"→ {v['puntaje']:.3f} %/dB")
print("  Ganadora:", ganadora)

fig, axs = plt.subplots(1, 2, figsize=(11, 3.9))
uu = np.logspace(-2.2, 0, 800)
orden = sorted(fam, key=lambda k: fam[k]["puntaje"])
for i, k in enumerate(orden):
    h = c.FAMILIAS[k]
    est = "-" if fam[k]["monotona"] else ":"
    lw = 2.4 if k == ganadora else 1.3
    col = MENTA if k == ganadora else SERIES[i % len(SERIES)]
    g = db(c.densidad(uu, G_FAM, h) / uu)
    axs[0].plot(db(uu), g, est, color=col, lw=lw, label=k + ("" if fam[k]["monotona"] else " (no monótona)"))
    axs[1].plot(fam[k]["ganancia_db"], fam[k]["thd_pct"], est, marker="o", ms=3.5, color=col, lw=lw)
axs[0].set(title=f"Ganancia de Densidad g(|x|/C) — G = +{G_FAM:g} dB",
           xlabel="Nivel de entrada (dB re techo)", ylabel="Ganancia (dB)", xlim=(-44, 0))
axs[0].legend(loc="lower left", ncol=1)
axs[1].set(title="THD vs ganancia RMS (senoide, −24 a 0 dB re techo)",
           xlabel="Ganancia RMS ganada (dB)", ylabel="THD (%)")
axs[1].annotate("mejor: abajo a la derecha", xy=(0.98, 0.95), xycoords="axes fraction",
                ha="right", color=TXT2, fontsize=8.5)
guardar(fig, "01_densidad_familias.png")

H = c.FAMILIAS[ganadora]

# ============================================================ 2. Curvas fusionadas por modo
fig, axs = plt.subplots(1, 3, figsize=(13, 4))
G_DEMO = 3.0
xin = np.linspace(-30, 12, 1000)
ul = np.linspace(0, 1.8, 2000)
for i, modo in enumerate(c.MODOS):
    col = SERIES[i]
    y = c.nucleo(10 ** (xin / 20), G_DEMO, modo, H)
    axs[0].plot(xin, db(y), color=col, label=modo)
    axs[1].plot(ul, c.nucleo(ul, G_DEMO, modo, H), color=col, label=modo)
    axs[2].plot(ul, np.gradient(c.nucleo(ul, G_DEMO, modo, H), ul), color=col, label=modo)
axs[0].plot(xin, xin, color=TXT2, lw=0.8, ls="--")
axs[0].axhline(0, color=CORAL, lw=0.9)
axs[0].text(-29, 1.0, "techo", color=CORAL, fontsize=8.5)
axs[0].set(title=f"Transferencia (dB) — Densidad +{G_DEMO:g} dB", xlabel="Entrada (dB re techo)",
           ylabel="Salida (dB re techo)", ylim=(-30, 6))
axs[1].plot(ul, ul, color=TXT2, lw=0.8, ls="--")
axs[1].set(title="Transferencia lineal (u = x / C)", xlabel="Entrada u", ylabel="Salida f(u)", ylim=(0, 1.6))
axs[2].set(title="Pendiente f'(u) (continua salvo Duro)", xlabel="Entrada u", ylabel="f'(u)")
axs[0].legend(loc="lower right"); axs[1].legend(loc="lower right"); axs[2].legend(loc="upper right")
guardar(fig, "02_curvas_modos.png")

# asimetría de Suave: armónicos pares
arms = {}
for modo in ("Suave", "Firme"):
    n = 8192
    th = 2 * np.pi * np.arange(n) / n
    Y = np.abs(np.fft.rfft(c.nucleo(1.4 * np.sin(th), 0.0, modo, H))) / (n / 2)
    arms[modo] = {f"H{k}": float(db(Y[k] / Y[1])) for k in range(2, 6)}
R["armonicos_+3dB_sobre_techo"] = arms
print("Armónicos a +2.9 dB sobre el techo:", json.dumps(arms, indent=None))

# ============================================================ 3. Tabla: tamaño vs error
res_tab = {}
uu = np.linspace(-2.5, 2.5, 400001)
for modo in ("Suave", "Firme", "Duro"):
    ref = c.nucleo(uu, 6.0, modo, H)
    res_tab[modo] = {}
    for ns in (64, 128, 256, 512, 1024):
        T = c.tabla_nucleo(6.0, modo, ns, H)
        res_tab[modo][ns] = float(db(np.max(np.abs(T.f(uu) - ref))))
R["tabla_error_max_db"] = res_tab
print("Error máx. de tabla (dB re techo):", res_tab)

# ============================================================ 4. Aliasing
FS, N, PAD = 48000.0, 1 << 15, 4096


def sar(y, k):
    """Relación señal/alias: potencia en armónicos legítimos (m·f0 < fs/2) contra componentes inarmónicas en 0–20 kHz."""
    Y = np.abs(np.fft.rfft(y)) ** 2
    Y[0] = 0
    mk = np.arange(1, (N // 2) // k + 1) * k
    mk = mk[mk < N // 2]
    s = Y[mk].sum()
    banda = int(20000 * N / FS)          # alias audible: solo 0–20 kHz
    a = Y[:banda].sum() - Y[mk[mk < banda]].sum()
    return 10 * np.log10(s / max(a, 1e-300)), Y


def tono(f0):
    k = int(round(f0 * N / FS))
    k += (k % 2 == 0)
    n = np.arange(N + 2 * PAD)          # relleno a ambos lados: sin bordes de convolución
    return k, np.sin(2 * np.pi * k * n / N)


AMP = 2.0  # +6 dB sobre el techo
FREQS = np.round(np.logspace(np.log10(1000), np.log10(20000), 22))
CALIDADES = [("Eco 1x", 1), ("Normal 2x", 2), ("Alta 4x", 4), ("Máxima 8x", 8)]
CASOS = [("Duro", 0.0), ("Firme", 3.0)]
alias = {}
for modo, G in CASOS:
    T = c.tabla_nucleo(G, modo, 512, H)
    alias[modo] = {}
    for nom, fac in CALIDADES:
        for ad in (False, True):
            key = f"{nom}{' + ADAA' if ad else ''}"
            v = []
            for f0 in FREQS:
                k, x = tono(f0)
                y = c.procesar(AMP * x, T, fac, ad)[PAD:PAD + N]
                v.append(sar(y, k)[0])
            alias[modo][key] = v
            print(f"  alias {modo:5s} {key:20s} SAR min={min(v):6.1f} dB  mediana={np.median(v):6.1f} dB")
R["aliasing"] = {"frecuencias_hz": list(FREQS), "amplitud_db_sobre_techo": 6.02, "sar_db": alias}

fig, axs = plt.subplots(1, 2, figsize=(12, 4.2), sharey=True)
for ax, (modo, G) in zip(axs, CASOS):
    for i, (nom, fac) in enumerate(CALIDADES):
        col = SERIES[i]
        ax.plot(FREQS / 1000, alias[modo][f"{nom} + ADAA"], "-", color=col, label=f"{nom} + ADAA")
        ax.plot(FREQS / 1000, alias[modo][nom], "--", color=col, lw=1.1, alpha=0.85, label=f"{nom} sin ADAA")
    ax.set_xscale("log")
    ax.set_xticks([1, 2, 5, 10, 20]); ax.set_xticklabels(["1k", "2k", "5k", "10k", "20k"])
    ax.set(title=f"Relación señal/alias — {modo}, Densidad +{G:g} dB, +6 dB sobre techo",
           xlabel="Frecuencia de la senoide (Hz)")
axs[0].set_ylabel("Señal / alias (dB, más alto = mejor)")
h_, l_ = axs[0].get_legend_handles_labels()
fig.legend(h_, l_, loc="lower center", ncol=4, bbox_to_anchor=(0.5, -0.17))
guardar(fig, "03_aliasing_sar.png")

# espectros de ejemplo
fig, axs = plt.subplots(2, 2, figsize=(12, 6), sharex=True, sharey=True)
T = c.tabla_nucleo(0.0, "Duro", 512, H)
f0 = 7001
k, x = tono(f0)
for ax, (tit, fac, ad) in zip(axs.flat, [("Eco 1x sin ADAA", 1, False), ("Eco 1x + ADAA", 1, True),
                                         ("Normal 2x + ADAA", 2, True), ("Máxima 8x + ADAA", 8, True)]):
    y = c.procesar(AMP * x, T, fac, ad)[PAD:PAD + N]
    s, Y = sar(y, k)
    Ydb = 10 * np.log10(Y / Y[k] + 1e-30)
    fr = np.arange(len(Y)) * FS / N / 1000
    mk = np.arange(1, (N // 2) // k + 1) * k
    mk = mk[mk < N // 2]
    ax.plot(fr, Ydb, color=CORAL, lw=0.7, label="alias (inarmónico)")
    mk = mk[Ydb[mk] > -150]
    ax.vlines(fr[mk], -200, Ydb[mk], color=MENTA, lw=2.2, label="armónicos legítimos")
    ax.axvspan(20, 24, color=LIN, alpha=0.6, lw=0)
    ax.set(title=f"{tit} — señal/alias {s:.1f} dB", ylim=(-160, 5), xlim=(0, 24))
    ax.grid(True)
axs[1, 0].set(xlabel="Frecuencia (kHz)", ylabel="dB re fundamental")
axs[1, 1].set(xlabel="Frecuencia (kHz)")
axs[0, 0].set(ylabel="dB re fundamental")
axs[0, 0].legend(loc="upper right")
fig.suptitle(f"Duro, senoide 7 kHz a +6 dB sobre el techo", x=0.01, ha="left", color=TXT, fontsize=11,
             fontweight="semibold")
guardar(fig, "04_espectros_duro_7k.png")

# ============================================================ 5. Precisión float32 del ADAA
prec = {}
fsr = 96000.0
n = np.arange(1 << 15)
for nivel_db in (-40.0, -12.0, 6.0):
    x = (10 ** (nivel_db / 20) * np.sin(2 * np.pi * 997 * n / fsr))
    T64 = c.tabla_nucleo(4.0, "Firme", 512, H)
    T32 = T64.astype(np.float32)
    ref = T64.adaa_tramo(x)
    x32 = x.astype(np.float32)
    e = {}
    for nom, y in (("ingenuo ε=1e-5", T32.adaa_ingenuo(x32, np.float32(1e-5))),
                   ("ingenuo ε=1e-3", T32.adaa_ingenuo(x32, np.float32(1e-3))),
                   ("por tramo", T32.adaa_tramo(x32))):
        err = y.astype(np.float64) - ref
        e[nom] = float(db(np.sqrt(np.mean(err ** 2)) / np.sqrt(np.mean(ref ** 2))))
    prec[f"{nivel_db:+g} dB re techo"] = e
R["adaa_float32_error_db"] = prec
print("Error ADAA float32 (dB re señal):", json.dumps(prec, indent=1, ensure_ascii=False))

# ============================================================ 6. Costos de filtros
costos = {}
for nom, fac in CALIDADES[1:]:
    hs = c.etapas(fac)
    costos[nom] = [dict(fs_khz=48 * 2 ** (i + 1), taps=len(h), no_nulos=int(np.sum(np.abs(h) > 0)))
                   for i, h in enumerate(hs)]
R["halfband_referencia"] = costos
print("Half-band de referencia:", costos)

with open(os.path.join(AQUI, "..", "docs", "fase1", "resultados.json"), "w") as fh:
    json.dump(R, fh, indent=1, ensure_ascii=False)
print("listo")
