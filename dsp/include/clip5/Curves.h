// clip5 — diseño de las curvas estáticas (doble precisión; solo se usa al generar tablas).
// Todo normalizado al techo: u = x / C. Fórmulas idénticas a proto/clip5_curvas.py.
#pragma once

namespace clip5
{
enum class ClipMode : int { Suave = 0, Firme = 1, Duro = 2, Apagado = 3 };

struct CurveParams
{
    float densityDb = 0.0f;           // 0 … +6 dB
    ClipMode mode = ClipMode::Firme;

    bool operator== (const CurveParams& o) const noexcept { return densityDb == o.densityDb && mode == o.mode; }
    bool operator!= (const CurveParams& o) const noexcept { return ! (*this == o); }
};

namespace curves
{
    constexpr double kMaxDensityDb = 6.0;

    /** Ganancia de Densidad g(a) para a = |u| ∈ [0, 1]: h(a) = (1 − a²)², familia
        "potencia p=2" elegida en la fase 1 (menor THD por dB de RMS ganado entre las monótonas). */
    double densityGain (double a, double densityDb) noexcept;
    double density (double u, double densityDb) noexcept;

    /** Rodilla C² de ancho w centrada en 1: lineal hasta 1 − w/2, satura exactamente en 1. */
    double knee (double v, double w) noexcept;
    double clip (double v, ClipMode mode) noexcept;

    /** Curva fusionada f(u) = recorte(densidad(u)). */
    double shaper (double u, double densityDb, ClipMode mode) noexcept;

    /** Pendiente de f fuera de la tabla (|u| > 2): 0 con recorte, 1 en Apagado. */
    double outerSlope (ClipMode mode) noexcept;

    /** Anchos de rodilla (positivo, negativo). */
    void kneeWidths (ClipMode mode, double& wPos, double& wNeg) noexcept;
}
} // namespace clip5
