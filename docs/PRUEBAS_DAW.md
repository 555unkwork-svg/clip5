# clip5 — verificación en DAWs y comparativa de CPU

Estas pruebas requieren las DAWs y una máquina física (no se pueden hacer en la CI).
Registrar versión de DAW, sistema operativo, CPU, frecuencia de muestreo y tamaño de buffer.

## Matriz

| DAW | Formato | SO |
|---|---|---|
| Reaper 7 | VST3, CLAP | macOS, Windows |
| Ableton Live 12 | VST3, AU | macOS, Windows |
| Logic Pro | AU | macOS |
| Pro Tools | AAX (requiere SDK + firma PACE) | macOS, Windows |
| FL Studio 2025 | VST3, CLAP | macOS, Windows |
| Cubase 14 | VST3 | macOS, Windows |

## Lista por DAW

1. Carga, abrir/cerrar el editor 20 veces, escalas 75 % y 200 %.
2. **Compensación de latencia**: pista duplicada con clip5 (Apagado, Densidad 0, Húmedo 0, Seco −∞,
   Techo 0, Techo estricto apagado) e invertida en fase contra la original → silencio.
   Repetir cambiando Calidad y Fase durante la reproducción (la DAW debe actualizar la latencia).
3. **Bypass del host**: activar y desactivar durante la reproducción, sin saltos ni desalineación.
4. **Automatización**: grabar Entrada, Densidad, Techo y Suavizado; reproducir: sin clics.
5. **Render offline**: exportar con Calidad al exportar = Máxima; comparar dos exportaciones
   (deben ser idénticas bit a bit).
6. **Estado**: guardar el proyecto, cerrar y reabrir → mismos valores, preset y A/B enlazado.
7. **Reloj de recorte**: con transporte, la posición coincide con la línea de tiempo de la DAW.
8. **Mono**: insertar en una pista mono (si la DAW lo permite).
9. **Frecuencias de muestreo**: 44,1 / 48 / 88,2 / 96 / 192 kHz.
10. **Techo estricto**: medir la salida con el medidor true peak de la DAW (o con un medidor
    externo BS.1770): nunca por encima del techo.

## Comparativa de CPU

1. Misma máquina, misma sesión: 50 pistas estéreo de audio a 48 kHz y buffer de 128.
2. Insertar una instancia por pista de: clip5 (Normal, Techo estricto activo) y 2–3 clippers
   comerciales en su calidad por defecto y en su calidad máxima.
3. Registrar el medidor de rendimiento de la DAW (Reaper: *Performance Meter*, % de CPU de FX
   por pista), con 3 corridas de 60 s cada una; informar la mediana.
4. Repetir con `clip5_bench` en la misma máquina para relacionar la cifra con el motor aislado.
