"""Verificaciones de las curvas (las mismas que luego irán a Catch2 en C++).
Ejecutar:  python3 proto/test_curvas.py"""
import sys
import numpy as np
import clip5_curvas as c

fallos = []


def check(nombre, ok, detalle=""):
    print(f"  [{'OK ' if ok else 'FALLA'}] {nombre} {detalle}")
    if not ok:
        fallos.append(nombre)


u = np.linspace(-3, 3, 600001)
for G in (0.0, 3.0, 6.0):
    for modo in c.MODOS:
        print(f"G=+{G:g} dB, modo {modo}")
        f = c.nucleo(u, G, modo)
        df = np.diff(f)
        check("monotonía (no decreciente)", np.all(df >= -1e-15), f"min Δ={df.min():.2e}")
        fd1 = c.densidad(np.array([1.0, -1.0]), G)
        check("f_d(C) = C", np.allclose(fd1, [1, -1], atol=1e-12))
        if modo in ("Duro", "Apagado"):
            f1 = c.nucleo(np.array([1.0]), G, modo)[0]
            check("f(C) = C", abs(f1 - 1) < 1e-12, f"f(1)={f1:.6f}")
        else:
            f1 = c.nucleo(np.array([1.0]), G, modo)[0]
            print(f"         (rodilla) f(C) = {20*np.log10(f1):+.3f} dB re C; máx |f| = {np.abs(f).max():.6f}")
            check("salida ≤ techo", np.abs(f).max() <= 1 + 1e-12)
        x = 1e-4
        gain = 20 * np.log10(c.nucleo(np.array([x]), G, modo)[0] / x)
        check("ganancia de bajo nivel = G", abs(gain - G) < 1e-3, f"{gain:+.5f} dB")
        # continuidad de f' (excepto quiebre del modo Duro en |v|=1)
        d = np.gradient(f, u)
        salto = np.abs(np.diff(d))
        if modo == "Duro":
            v = c.densidad(u, G)
            mask = np.abs(np.abs(v[1:]) - 1) > 1e-3
            salto = salto[mask]
        check("f' continua", salto.max() < 1e-2, f"máx salto={salto.max():.2e}")
        # tabla: F1 consistente con f (derivada numérica) y error de interpolación
        T = c.tabla_nucleo(G, modo)
        uu = np.linspace(-2.5, 2.5, 200001)
        err = np.max(np.abs(T.f(uu) - c.nucleo(uu, G, modo)))
        check("tabla ≈ curva", err < 1e-6, f"err máx={err:.1e}")
        dF = np.gradient(T.F(uu), uu)
        errF = np.max(np.abs(dF - T.f(uu))[1:-1])
        check("dF1/du = f", errF < 1e-3, f"err máx={errF:.1e}")

print()
print("RESULTADO:", "todo OK" if not fallos else f"{len(fallos)} fallos")
sys.exit(1 if fallos else 0)
