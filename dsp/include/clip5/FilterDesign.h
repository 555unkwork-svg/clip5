// clip5 — diseño de filtros (doble precisión). Se ejecuta en prepare(), nunca en el hilo de audio.
#pragma once

#include <functional>
#include <vector>

namespace clip5::design
{
/** Half-band FIR de fase lineal (ventana de Kaiser), corte en fsHigh/4.
    Banda de rechazo desde fsHigh/2 − fPass. Largo forzado a 4m+3. Suma = 1. */
std::vector<double> kaiserHalfband (double fsHigh, double fPass, double attenDb);

/** Coeficientes allpass de un half-band IIR polifásico (fase mínima), método elíptico
    (algoritmo de dominio público de L. de Soras, HIIR). `transition` = (fsHigh/4 − fPass)/fsHigh.
    Elige la menor cantidad de coeficientes que alcanza `attenDb` (medido). */
std::vector<double> iirHalfband (double transition, double attenDb);

/** Respuesta en módulo del half-band IIR a la frecuencia normalizada f (0 … 0.5 de fsHigh). */
double iirHalfbandMagnitude (const std::vector<double>& a, double f);

/** FIR de mínimos cuadrados con retardo de grupo fraccional D (muestras) y módulo objetivo
    target(f) en la banda de paso; cero en la banda de rechazo. Fase ≈ lineal de retardo D. */
std::vector<double> leastSquaresFir (int taps, double fs, double fPass, double fStop,
                                     const std::function<double (double)>& target, double D,
                                     double stopWeight);

/** Curvas de Tono (dB) — plano bajo 1 kHz: −D·r²/(1+r²), r = f/f0. */
double tonoDb (int tono, double f);

/** Interpolador 4x para true peak: 96 taps (4 fases × 24), mínimos cuadrados. */
std::vector<double> truePeakInterpolator();

struct BiquadCoefs { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

BiquadCoefs highShelf (double fs, double f0, double gainDb, double q);
double biquadMagnitudeDb (const BiquadCoefs& c, double fs, double f);

/** Ajusta un high-shelf (f0, ganancia) a un objetivo en dB sobre 1–19 kHz (mínimo máximo error). */
BiquadCoefs fitHighShelf (double fs, const std::function<double (double)>& targetDb, double* maxErrDb = nullptr);

/** Coeficientes de los dos filtros K (ITU-R BS.1770-4) para cualquier fs. */
void kWeighting (double fs, BiquadCoefs& shelf, BiquadCoefs& highpass);
} // namespace clip5::design
