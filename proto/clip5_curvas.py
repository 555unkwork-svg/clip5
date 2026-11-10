"""
clip5 — prototipo de las curvas del núcleo no lineal fusionado (Densidad + Recorte).

Todo está normalizado al techo: u = x / C. La salida real es y = C · f(u).
Por eso la tabla NO depende del Techo: solo se regenera al cambiar Densidad o Modo.

Contenido:
  - Familias de Densidad  f_d(u) = u · g(|u|),  g(0)=10^(G/20), g(1)=1, g'(1)=0
  - Modos de recorte (rodilla C2 de ancho w, asimétrica en Suave)
  - Tabla por tramos cúbicos de Hermite + antiderivada exacta por tramo
  - ADAA de 1.er orden: (a) "ingenuo" (diferencia de F1 + ε) y
                        (b) "por tramo" (media exacta del polinomio, sin cancelación)
  - Sobremuestreo con half-band FIR (Kaiser) en cascada (referencia de análisis)
"""
from __future__ import annotations

import numpy as np
from scipy import signal

# --------------------------------------------------------------------------------------
# Densidad: h(u) va de 1 (u=0) a 0 (u=1) con h'(1)=0  →  g(u) = 1 + (g0-1)·h(u)
# --------------------------------------------------------------------------------------

def h_poli(u):
    """Polinómica por tramos (smoothstep complementario): h = 1 - 3u² + 2u³."""
    u = np.clip(u, 0.0, 1.0)
    return 1.0 - 3.0 * u**2 + 2.0 * u**3


def h_pot(p):
    """Potencia con exponente de rodilla p: h = (1 - u^p)²."""
    def h(u):
        u = np.clip(u, 0.0, 1.0)
        return (1.0 - u**p) ** 2
    h.__name__ = f"pot_p{p:g}"
    return h


def h_exp(a, p):
    """Exponencial con exponente de rodilla p: h = E(u)·(1 - u^p),
    E(u) = (e^{-a u^p} - e^{-a}) / (1 - e^{-a}). Producto de dos ceros en u=1 → h'(1)=0."""
    def h(u):
        u = np.clip(u, 0.0, 1.0)
        w = u**p
        E = (np.exp(-a * w) - np.exp(-a)) / (1.0 - np.exp(-a))
        return E * (1.0 - w)
    h.__name__ = f"exp_a{a:g}_p{p:g}"
    return h


FAMILIAS = {
    "Polinómica (smoothstep)": h_poli,
    "Potencia p=1": h_pot(1.0),
    "Potencia p=1.5": h_pot(1.5),
    "Potencia p=2": h_pot(2.0),
    "Potencia p=3": h_pot(3.0),
    "Exponencial a=2 p=1": h_exp(2.0, 1.0),
    "Exponencial a=2 p=2": h_exp(2.0, 2.0),
    "Exponencial a=4 p=2": h_exp(4.0, 2.0),
}


def densidad(u, G_db, h=h_poli):
    """f_d(u) = u·g(|u|). Para |u|≥1 es la identidad (ganancia 0 dB desde el techo)."""
    g0 = 10.0 ** (G_db / 20.0)
    a = np.abs(u)
    g = np.where(a < 1.0, 1.0 + (g0 - 1.0) * h(a), 1.0)
    return u * g


# --------------------------------------------------------------------------------------
# Recorte: rodilla C2. Lineal hasta 1-w/2, saturación exacta en 1 a partir de 1+w/2.
#   y'(v) = 1 - smoothstep(t), t = (v - (1 - w/2)) / w
#   y = a + w·(t - t³ + t⁴/2)
# --------------------------------------------------------------------------------------

def rodilla(v, w):
    if w <= 0:
        return np.minimum(v, 1.0)
    a = 1.0 - w / 2.0
    t = np.clip((v - a) / w, 0.0, 1.0)
    knee = a + w * (t - t**3 + 0.5 * t**4)
    return np.where(v <= a, v, knee)


MODOS = {
    #            w(+)   w(−)   pendiente fuera de tabla
    "Suave":   (0.50,  0.70,  0.0),   # rodilla amplia + asimetría leve → pares
    "Firme":   (0.16,  0.16,  0.0),   # rodilla estrecha simétrica → impares
    "Duro":    (0.0,   0.0,   0.0),   # lineal hasta C, recorte duro
    "Apagado": (None,  None,  1.0),   # sin recorte
}


def recorte(v, modo):
    wp, wn, _ = MODOS[modo]
    if wp is None:
        return v
    return np.where(v >= 0, rodilla(v, wp), -rodilla(-v, wn))


def nucleo(u, G_db, modo, h=h_poli):
    """Curva estática fusionada f(u) = recorte(densidad(u))."""
    return recorte(densidad(u, G_db, h), modo)


# --------------------------------------------------------------------------------------
# Tabla por tramos cúbicos (Hermite con derivadas laterales) + antiderivada exacta
# --------------------------------------------------------------------------------------

class Tabla:
    """Tabla uniforme en [lo, hi]. Fuera del rango: extrapolación lineal exacta
    (pendiente 0 en los modos con recorte, 1 en Apagado) — todos los modos son
    exactamente lineales para |u| ≥ 1 + w/2 ≤ 1.35, así que [-2, 2] basta."""

    def __init__(self, f, nseg=512, lo=-2.0, hi=2.0, pendiente_ext=0.0):
        self.lo, self.hi, self.n = lo, hi, nseg
        self.h = (hi - lo) / nseg
        x = lo + self.h * np.arange(nseg + 1)
        d = 1e-5
        fx = f(x)
        # derivadas laterales (Richardson, 2.º orden) → los quiebres en nodos se respetan
        dp = (-3 * fx + 4 * f(x + d) - f(x + 2 * d)) / (2 * d)
        dm = (3 * fx - 4 * f(x - d) + f(x - 2 * d)) / (2 * d)
        f0, f1 = fx[:-1], fx[1:]
        d0, d1 = dp[:-1] * self.h, dm[1:] * self.h
        self.c = np.stack([f0, d0, 3 * (f1 - f0) - 2 * d0 - d1, 2 * (f0 - f1) + d0 + d1], 1)
        seg_int = self.h * (self.c[:, 0] + self.c[:, 1] / 2 + self.c[:, 2] / 3 + self.c[:, 3] / 4)
        P = np.concatenate([[0.0], np.cumsum(seg_int)])
        # F1 anclada en u=0 (F1(0)=0): cerca de cero |F1| ~ u²/2 es diminuta y la resta
        # F1(u)-F1(u₋₁) en float32 no pierde los bits de las señales de bajo nivel.
        self.mitad = nseg // 2
        assert abs(lo + hi) < 1e-12 and nseg % 2 == 0
        self.P = P - P[self.mitad]
        self.f_lo, self.f_hi, self.s = fx[0], fx[-1], pendiente_ext

    def astype(self, dt):
        t = object.__new__(Tabla)
        t.__dict__ = {k: (v.astype(dt) if isinstance(v, np.ndarray) else (dt(v) if isinstance(v, float) else v))
                      for k, v in self.__dict__.items()}
        return t

    def _seg(self, u):
        # índice relativo a u=0 (h = potencia de 2 → u/h exacto; t = q - floor(q) exacto)
        q = u / u.dtype.type(self.h)
        i = np.floor(q).astype(np.int64) + self.mitad
        i = np.clip(i, -1, self.n)          # -1 = izquierda, n = derecha
        ic = np.clip(i, 0, self.n - 1)
        t = (q - (ic - self.mitad)).astype(u.dtype)
        return i, ic, t

    def f(self, u):
        i, ic, t = self._seg(u)
        c = self.c[ic]
        y = c[:, 0] + t * (c[:, 1] + t * (c[:, 2] + t * c[:, 3]))
        y = np.where(i < 0, self.f_lo + self.s * (u - self.lo), y)
        y = np.where(i >= self.n, self.f_hi + self.s * (u - self.hi), y)
        return y

    def F(self, u):
        i, ic, t = self._seg(u)
        c = self.c[ic]
        poly = t * (c[:, 0] + t * (c[:, 1] / 2 + t * (c[:, 2] / 3 + t * c[:, 3] / 4)))
        Y = self.P[ic] + self.h * poly
        dl, dr = u - self.lo, u - self.hi
        Y = np.where(i < 0, self.P[0] + self.f_lo * dl + self.s * dl * dl / 2, Y)
        Y = np.where(i >= self.n, self.P[-1] + self.f_hi * dr + self.s * dr * dr / 2, Y)
        return Y

    # ---------------------------------------------------------------- ADAA 1.er orden
    def adaa_ingenuo(self, u, eps=1e-5):
        """(F(u) - F(u₋₁)) / (u - u₋₁), con fallback f((u+u₋₁)/2) si |Δ| < ε."""
        u1 = np.concatenate([u[:1], u[:-1]])
        du = u - u1
        small = np.abs(du) < eps
        safe = np.where(small, 1, du).astype(u.dtype)
        y = (self.F(u) - self.F(u1)) / safe
        return np.where(small, self.f((u + u1) / 2), y)

    def adaa_tramo(self, u):
        """Si u y u₋₁ caen en el mismo tramo, la media del polinomio se calcula en forma
        factorizada (sin restar F1 → sin cancelación, sin ε). Si no, diferencia de F1
        (Δ ≥ ~1 tramo salvo al cruzar un nodo; ahí también se usa la forma exacta
        partida en el nodo)."""
        u1 = np.concatenate([u[:1], u[:-1]])
        i, ic, t = self._seg(u)
        j, jc, s = self._seg(u1)
        c = self.c[ic]
        mismo = i == j
        ts, t2, s2 = t * s, t * t, s * s
        media = (c[:, 0] + c[:, 1] * (t + s) / 2 + c[:, 2] * (t2 + ts + s2) / 3
                 + c[:, 3] * (t2 * t + t2 * s + t * s2 + s2 * s) / 4)
        # fuera de tabla (lineal): media = f(punto medio), exacta
        fuera = (i < 0) | (i >= self.n)
        media = np.where(fuera, self.f((u + u1) / 2), media)
        du = u - u1
        safe = np.where(mismo, 1, du).astype(u.dtype)
        y_dif = (self.F(u) - self.F(u1)) / safe
        # tramos vecinos: partir en el nodo b y ponderar dos medias exactas
        vecino = (np.abs(i - j) == 1) & ~mismo
        if np.any(vecino):
            k = np.nonzero(vecino)[0]
            a, b = u1[k], u[k]
            nodo = u.dtype.type(self.h) * (np.maximum(i[k], j[k]) - self.mitad).astype(u.dtype)
            m1 = self._media_seg(a, nodo)
            m2 = self._media_seg(nodo, b)
            w = (nodo - a) / (b - a)
            y_dif[k] = w * m1 + (1 - w) * m2
        return np.where(mismo, media, y_dif)

    def _media_seg(self, a, b):
        """Media exacta de f en [a, b] con a y b en el mismo tramo (o borde)."""
        mid = (a + b) / 2
        i, ic, _ = self._seg(mid)
        base = a.dtype.type(self.h) * (ic - self.mitad).astype(a.dtype)
        t, s = (a - base) / self.h, (b - base) / self.h
        c = self.c[ic]
        ts, t2, s2 = t * s, t * t, s * s
        m = (c[:, 0] + c[:, 1] * (t + s) / 2 + c[:, 2] * (t2 + ts + s2) / 3
             + c[:, 3] * (t2 * t + t2 * s + t * s2 + s2 * s) / 4)
        fuera = (i < 0) | (i >= self.n)
        return np.where(fuera, self.f(mid), m)


def tabla_nucleo(G_db, modo, nseg=512, h=h_poli):
    return Tabla(lambda u: nucleo(u, G_db, modo, h), nseg=nseg, pendiente_ext=MODOS[modo][2])


# --------------------------------------------------------------------------------------
# Sobremuestreo de referencia: half-band FIR (Kaiser) en cascada, fase lineal
# --------------------------------------------------------------------------------------

def halfband(fs_alta, f_pasa=20000.0, aten_db=110.0):
    """Diseña un half-band (corte fs/4) para la tasa fs_alta. Banda de rechazo empieza
    en fs_alta/2 - f_pasa (la imagen del audio). Devuelve h (suma = 1)."""
    f_rech = fs_alta / 2 - f_pasa
    ancho = (f_rech - f_pasa) / (fs_alta / 2)
    n, beta = signal.kaiserord(aten_db, ancho)
    n = int(n)
    n += (3 - n % 4) % 4                # forma 4m+3 → centro en posición impar par-simétrica
    m = np.arange(n) - (n - 1) / 2
    hb = 0.5 * np.sinc(m / 2) * np.kaiser(n, beta)
    hb[np.abs(m) % 2 == 0] = 0.0
    hb[(n - 1) // 2] = 0.5
    return hb / hb.sum()


def etapas(factor, fs=48000.0):
    hs, f = [], fs
    while factor > 1:
        f *= 2
        hs.append(halfband(f))
        factor //= 2
    return hs


def sobre(x, hs):
    for hb in hs:
        y = np.zeros(2 * len(x))
        y[::2] = x
        x = signal.fftconvolve(y, 2 * hb, mode="same")
    return x


def sub(x, hs):
    for hb in reversed(hs):
        x = signal.fftconvolve(x, hb, mode="same")[::2]
    return x


def procesar(x, tabla, factor=1, adaa=True):
    """x ya normalizado al techo (u). Devuelve f(u) a tasa base."""
    hs = etapas(factor)
    u = sobre(x, hs)
    y = tabla.adaa_tramo(u) if adaa else tabla.f(u)
    return sub(y, hs)
