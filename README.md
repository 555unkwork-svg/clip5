# clip5

Clipper de mastering de alta eficiencia: ganancia no lineal (Densidad), recorte con
cuatro modos, contorno dinámico de agudos (Suavizado) y mezclador paralelo. Interfaz en español.

## Estado: Fase 1 (pendiente de aprobación)

| Entregable | Ruta |
|---|---|
| Informe (cadena, curvas, aliasing, CPU, preguntas) | `docs/fase1/informe.html` |
| Prototipo de curvas + tabla + ADAA + sobremuestreo | `proto/clip5_curvas.py` |
| Verificaciones de curvas (futuro Catch2) | `proto/test_curvas.py` |
| Análisis y figuras | `proto/analisis.py` → `docs/fase1/figuras/`, `docs/fase1/resultados.json` |
| Mockup interactivo de la UI | `mockup/clip5_ui.html` |

## Reproducir

```bash
pip install numpy scipy matplotlib
python3 proto/test_curvas.py
python3 proto/analisis.py
python3 docs/fase1/construir_informe.py
```
