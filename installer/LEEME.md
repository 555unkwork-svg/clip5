# clip5 — Manual de usuario

**Versión 0.9.0 (beta)** · Clipper de mastering con Densidad, Suavizado y mezcla paralela

---

## Índice

1. [Qué es clip5](#1-qué-es-clip5)
2. [Requisitos](#2-requisitos)
3. [Instalación](#3-instalación)
4. [Primeros pasos](#4-primeros-pasos)
5. [La interfaz](#5-la-interfaz)
6. [Controles](#6-controles)
7. [Panel Avanzado](#7-panel-avanzado)
8. [Medición y Reloj de recorte](#8-medición-y-reloj-de-recorte)
9. [Calidad y latencia](#9-calidad-y-latencia)
10. [Flujos de trabajo recomendados](#10-flujos-de-trabajo-recomendados)
11. [Presets de fábrica](#11-presets-de-fábrica)
12. [Atajos](#12-atajos)
13. [Solución de problemas](#13-solución-de-problemas)
14. [Especificaciones técnicas](#14-especificaciones-técnicas)
15. [Créditos y licencias](#15-créditos-y-licencias)

---

## 1. Qué es clip5

clip5 es un **clipper de mastering**: sube el volumen percibido de una mezcla recortando los
picos por encima de un **techo**, sin el bombeo de un compresor o limitador. Suma tres
herramientas para que el resultado suene limpio:

- **Densidad**: agrega cuerpo a los pasajes de bajo nivel sin tocar los picos.
- **Suavizado**: suaviza los agudos ásperos que genera el recorte, solo cuando hay recorte.
- **Mezcla paralela**: combina la señal procesada (HÚMEDO) con la original (SECO), alineadas
  a la muestra.

Todo se procesa con sobremuestreo y antialiasing (ADAA), con un consumo de CPU muy bajo.

## 2. Requisitos

| | Windows | macOS |
|---|---|---|
| Sistema | Windows 10 u 11 de 64 bits | macOS 10.13 o posterior (Intel y Apple Silicon) |
| Formatos | VST3, CLAP, aplicación independiente | AU, VST3, CLAP, aplicación independiente |
| DAW | cualquier DAW compatible con VST3 o CLAP de 64 bits | Logic Pro, Ableton Live, Reaper, Cubase, FL Studio, Studio One… |

> **Pro Tools (AAX)** todavía no está incluido en esta beta.

## 3. Instalación

### Windows

1. Abre `clip5-…-Windows-Setup.exe`.
2. Si aparece **"Windows protegió su PC"** (SmartScreen), haz clic en **Más información → Ejecutar
   de todas formas**. Aparece porque esta beta todavía no está firmada digitalmente.
3. Elige los formatos que quieres instalar (por defecto, todos) y sigue el asistente.
4. Abre tu DAW y vuelve a escanear los plugins si no aparece clip5.

Ubicaciones:

| Formato | Carpeta |
|---|---|
| VST3 | `C:\Program Files\Common Files\VST3\clip5.vst3` |
| CLAP | `C:\Program Files\Common Files\CLAP\clip5.clap` |
| Aplicación | `C:\Program Files\clip5\clip5.exe` (acceso en el menú Inicio) |

**Desinstalar:** Configuración → Aplicaciones → clip5 → Desinstalar.

### macOS

1. Abre `clip5-…-macOS.pkg`.
2. Si macOS dice que **no se puede abrir porque es de un desarrollador no identificado**:
   haz clic derecho (o Control+clic) sobre el archivo → **Abrir** → **Abrir**.
   En macOS 15 o posterior: **Ajustes del Sistema → Privacidad y seguridad → Abrir igualmente**.
   Esta beta todavía no está notarizada por Apple.
3. En **Personalizar** puedes elegir qué formatos instalar.
4. Abre tu DAW. Logic Pro valida los plugins AU automáticamente la primera vez.

Ubicaciones:

| Formato | Carpeta |
|---|---|
| AU | `/Library/Audio/Plug-Ins/Components/clip5.component` |
| VST3 | `/Library/Audio/Plug-Ins/VST3/clip5.vst3` |
| CLAP | `/Library/Audio/Plug-Ins/CLAP/clip5.clap` |
| Aplicación | `/Applications/clip5.app` |

**Desinstalar:** borra esos cuatro elementos (Finder → Ir → Ir a la carpeta… y pega la ruta).

## 4. Primeros pasos

1. Inserta clip5 en el **bus master**, **al final de la cadena** (o justo antes de tu limitador
   final, si usas uno).
2. Carga el preset **"Mastering pop"** o **"Transparente"** como punto de partida.
3. Ajusta **TECHO** al nivel máximo que quieres en la salida, por ejemplo **−1,0 dB** para
   plataformas de streaming.
4. Sube **ENTRADA** de a poco mientras miras el display: las marcas **coral** indican cuánto se
   está recortando. En mastering suelen sonar bien recortes de **1 a 3 dB** en los golpes.
5. Si el recorte suena áspero, sube **SUAVIZADO**. Si falta cuerpo en los pasajes suaves, sube
   **DENSIDAD**.
6. Compara con **A/B** (ver [Atajos](#12-atajos)) y activa **Ganancia unitaria** en Avanzado para
   comparar a igual volumen.

## 5. La interfaz

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ clip5 │ preset ‹ › │ A B ⇄ │ ↶ ↷ │      Eco Normal Alta Máx. │ CPU │ ⚙       │  barra superior
├───────────────────────────────────────────────────────┬──────────────────────┤
│                                                       │ ENTRADA · SALIDA     │
│   DISPLAY: historial de nivel, línea de techo,        │ (medidores y LUFS)   │
│   recorte en coral, Densidad en menta                 ├──────────────────────┤
│                                      ┌─────────────┐  │ RELOJ DE RECORTE     │
│                                      │Transferencia│  ├──────────────────────┤
│                                      └─────────────┘  │ MEZCLA · SALIDA      │
├───────────────────────────────────────────────────────┤ HÚMEDO ⛓ SECO SALIDA │
│ ENTRADA  DENSIDAD  TECHO  SUAVIZADO │ Suave Firme Duro Apagado │ Avanzado    │
└───────────────────────────────────────────────────────┴──────────────────────┘
```

- **Display**: muestra los últimos ~8 segundos. Gris: entrada. Línea marfil: salida.
  Coral: cuánto se recortó por encima del techo. Banda menta abajo: ganancia de Densidad aplicada.
- **Transferencia** (esquina): curva entrada/salida actual. La línea punteada es "sin cambios" y
  la línea coral es el techo.

## 6. Controles

| Control | Rango | Qué hace |
|---|---|---|
| **ENTRADA** | ±12 dB | Cuánto se empuja la señal contra el techo. Más entrada = más recorte y más volumen. **No afecta a la señal SECA.** |
| **DENSIDAD** | 0 a +6 dB | Sube los pasajes de bajo nivel hasta +6 dB y llega a 0 dB exactamente en el techo. Es instantánea: sin ataque ni release, sin bombeo. Da cuerpo y sostén. |
| **TECHO** | 0 a −36 dBFS | Nivel máximo de salida y punto donde empieza el recorte. Mueve con él todo el procesamiento (Densidad y Suavizado se referencian al techo). |
| **SUAVIZADO** | 0 a 100 % | Baja suavemente los medios-agudos (hasta −2 dB, o −3 dB en modo extendido) **solo** cuando la señal se acerca al techo o lo supera. Doma la aspereza del recorte sin oscurecer el resto. |
| **MODO** | Suave / Firme / Duro / Apagado | El carácter del recorte (ver abajo). |
| **HÚMEDO** | −∞ a +6 dB | Nivel de la señal procesada. |
| **SECO** | −∞ a +6 dB | Nivel de la señal original, alineada a la muestra con la procesada. |
| **⛓ (enlace)** | | Con el enlace activo, mover un fader mueve el otro como un crossfade de potencia constante. |
| **SALIDA** | ±12 dB | Ganancia final. Con **Enlace E/S** activo funciona como offset. |

### Modos de recorte

| Modo | Sonido | Úsalo para |
|---|---|---|
| **Suave** | Rodilla amplia con una leve asimetría: armónicos pares, más cálido | voces, acústicos, mezclas que necesitan calidez |
| **Firme** | Rodilla estrecha: armónicos impares, enfocado y transparente | mastering general (modo por defecto) |
| **Duro** | Lineal hasta el techo y luego recorte duro: el máximo de pegada | electrónica, transitorios de batería |
| **Apagado** | Sin recorte: solo Densidad y Suavizado | dar densidad sin recortar (con otro limitador después) |

Cada botón de modo muestra la forma real de su curva con la Densidad actual.

## 7. Panel Avanzado

Se abre con el botón **Avanzado**.

| Opción | Descripción |
|---|---|
| **Tono** | **Plano**, **Seda** (caída suave de agudos, −1 dB a 10 kHz) o **Terciopelo** (más marcada, −3 dB a 10 kHz). Por debajo de 1 kHz no cambia nada. No consume CPU extra. |
| **Techo estricto** | Garantiza que el **true peak** de la salida nunca supere el techo, incluso con mezcla paralela o Salida positiva. Agrega ~1,8 ms de latencia. **Desactívalo en usos paralelos con techo bajo**: también limitaría la señal SECA. |
| **Suavizado extendido** | Lleva el Suavizado hasta −3 dB. |
| **Enlace E/S** | Al subir ENTRADA, la señal procesada baja en la misma cantidad: escuchas cómo cambia el *carácter* del recorte sin que suba el volumen. La perilla SALIDA pasa a ser el offset (también con **Shift+arrastrar ENTRADA**). |
| **Calidad al exportar** | Calidad usada al renderizar o exportar. Por defecto: **Máxima**. "Igual" usa la misma que en tiempo real. |
| **Fase** | **Lineal** (por defecto): sin distorsión de fase; la mezcla con SECO es perfecta. **Mínima**: latencia de pocas muestras, útil para grabar o tocar en vivo. |
| **Ganancia unitaria** | Resta la ganancia de Densidad para comparar con y sin procesamiento a igual volumen. |

## 8. Medición y Reloj de recorte

**ENTRADA · SALIDA** (clic para reiniciar los máximos y el LUFS integrado):

| Lectura | Significado |
|---|---|
| Barras E / S | Nivel de pico izquierdo/derecho. La raya coral sobre las barras de salida marca el techo. |
| **Pico** | Pico de muestra máximo desde el último reinicio. |
| **TP** | True peak de salida (picos entre muestras, 4x, como en BS.1770-4). En coral si supera el techo. |
| **LUFS M / S / I** | Sonoridad momentánea (400 ms), de corto plazo (3 s) e integrada (desde el último reinicio), según ITU-R BS.1770-4 / EBU R128. |
| **Recorte** | dB recortados en este momento. |
| **Densidad** | Ganancia de Densidad aplicada en este momento. |

**RELOJ DE RECORTE**: muestra el **mayor recorte** de la sesión y **en qué minuto y segundo**
ocurrió. Con el transporte en marcha usa la posición de la DAW. Así encuentras al instante el
pasaje más exigente de la canción. Clic para reiniciar.

## 9. Calidad y latencia

| Calidad | Sobremuestreo | Cuándo usarla |
|---|---|---|
| **Eco** | 1x + ADAA | muchas instancias, sesiones pesadas |
| **Normal** | 2x + ADAA | por defecto: excelente relación calidad/CPU |
| **Alta** | 4x + ADAA | mastering exigente |
| **Máxima** | 8x + ADAA | exportación final (configurada por defecto en *Calidad al exportar*) |

Latencia a 48 kHz (muestras), compensada automáticamente por la DAW:

| | Eco | Normal | Alta | Máxima |
|---|---|---|---|---|
| Fase lineal | 104 | 146 | 153 | 156 |
| Fase mínima | 88 | 92 | 94 | 95 |
| Fase mínima sin Techo estricto | 0 | 4 | 6 | 7 |

Cambiar la calidad o la fase durante la reproducción produce un breve fundido (sin clics) y la
DAW recibe la nueva latencia.

## 10. Flujos de trabajo recomendados

**Mastering para streaming**
Preset *Mastering pop* → TECHO −1,0 dB → Techo estricto activado → sube ENTRADA hasta que el
Reloj de recorte marque 2–3 dB en los golpes más fuertes → SUAVIZADO 30–50 % si se endurecen
los agudos → mira LUFS I al final de la canción.

**Bus de batería en paralelo**
Preset *Bus de batería* → modo Duro → HÚMEDO 0 dB, SECO −6 dB (enlace ⛓ desactivado) → Techo
estricto **apagado** → ajusta ENTRADA para el ataque deseado.

**Densidad sin recorte**
Modo **Apagado**, DENSIDAD +3 a +5 dB, Ganancia unitaria activada para comparar a igual volumen.
Pon tu limitador habitual después de clip5.

**Ajustar el carácter sin engañarse por el volumen**
Activa **Enlace E/S**: al subir ENTRADA el volumen se mantiene y escuchas solo cómo cambia el
recorte.

**Comparar dos ajustes (A/B)**
Al abrir, A y B están enlazados (A = B). Pulsa **A**, ajusta; pulsa **B**, ajusta otra cosa;
alterna entre A y B para comparar. **⇄** copia el ajuste activo al otro y los vuelve a enlazar.

## 11. Presets de fábrica

| Preset | Entrada | Densidad | Techo | Modo | Suavizado | Otros |
|---|---|---|---|---|---|---|
| Transparente | 0 | 0 | −1,0 | Suave | 0 % | |
| Mastering pop | +3 | +2 | −1,0 | Firme | 35 % | |
| Densidad sin recorte | 0 | +4 | −0,3 | Apagado | 20 % | Ganancia unitaria |
| Bus de batería | +6 | +3 | −6,0 | Duro | 20 % | Seco −6, Techo estricto apagado |
| Rock denso | +5 | +4 | −1,0 | Firme | 50 % | Tono Seda |
| Electrónica al límite | +8 | +5 | −0,5 | Duro | 30 % | |
| Voz al frente | +4 | +3 | −3,0 | Suave | 60 % | Tono Terciopelo, Seco −10, Techo estricto apagado |
| Paralelo sucio | +10 | +6 | −12 | Duro | 0 % | Húmedo −6, Seco 0, Salida −3, Techo estricto apagado |

Los presets **no cambian** la Calidad ni la Fase.

**Presets propios:** menú de presets → **Guardar preset…**. Se guardan en
`Documentos/clip5/Presets` y aparecen en la sección *Usuario*. Se pueden copiar entre
computadoras.

## 12. Atajos

| Acción | Cómo |
|---|---|
| Ajuste fino | **Shift** + arrastrar (o rueda del mouse) |
| Volver al valor por defecto | **Ctrl+clic** (Windows) / **Cmd+clic** (macOS) |
| Escribir un valor | **Doble clic** sobre la perilla, escribir y **Enter** |
| Fader a 0 dB | Doble clic sobre el fader |
| Deshacer / Rehacer | **Ctrl/Cmd+Z** / **Ctrl/Cmd+Shift+Z**, o los botones ↶ ↷ |
| Preset anterior / siguiente | botones ‹ › |
| Reiniciar medidores / reloj | clic sobre el panel |
| Escala de la interfaz (75–200 %) y 30/60 fps | botón ⚙ |
| Redimensionar | arrastrar la esquina inferior derecha |

Pasa el mouse sobre cualquier control para ver una ayuda en español.

## 13. Solución de problemas

| Problema | Solución |
|---|---|
| La DAW no encuentra clip5 | Vuelve a escanear los plugins y verifica que la carpeta VST3/CLAP de la tabla de instalación esté en las rutas de búsqueda de la DAW. |
| Windows bloquea el instalador | "Más información → Ejecutar de todas formas" (beta sin firma). |
| macOS no deja abrir el .pkg | Clic derecho → Abrir, o Ajustes → Privacidad y seguridad → Abrir igualmente. |
| Logic no muestra clip5 | Logic → Ajustes → Plug-in Manager → Restablecer y volver a escanear. |
| La salida supera el techo | Activa **Techo estricto** (Avanzado). Sin él, SECO, HÚMEDO > 0 dB o SALIDA > 0 dB pueden superarlo. |
| La mezcla en paralelo suena "hueca" | Usa **Fase lineal**: la fase mínima no se alinea perfectamente con SECO en agudos. |
| Mucho uso de CPU | Baja la Calidad en tiempo real a Normal o Eco; la exportación sigue usando *Calidad al exportar*. |
| Se escucha un salto al cambiar la Calidad | Es el fundido de seguridad (~10 ms) mientras cambia la latencia; no queda en la exportación. |

## 14. Especificaciones técnicas

- Procesamiento interno en coma flotante de 32 bits; diseño de filtros en 64 bits.
- Núcleo no lineal fusionado (Densidad + Recorte) en tabla de 512 tramos cúbicos (error < −130 dB)
  con antialiasing ADAA de primer orden.
- Sobremuestreo: half-band FIR de fase lineal (≥ 105 dB) o half-band IIR de fase mínima (≥ 110 dB).
- Tono integrado en el filtro de decimación (rizado < 0,02 dB, rechazo > 100 dB).
- Techo estricto: limitador con anticipación guiado por true peak (4x + refinamiento parabólico).
- Medición: LUFS ITU-R BS.1770-4 (precisión ±0,1 LU), true peak de salida.
- Frecuencias de muestreo: 44,1 a 192 kHz. Mono y estéreo.
- Consumo de CPU por instancia estéreo en calidad Normal: objetivo de diseño ≤ 1 % de un núcleo
  (medido: 1,1 % en un servidor de 2,1 GHz; en procesadores de escritorio actuales se espera
  bastante menos).

## 15. Créditos y licencias

clip5 © 2026. Todos los derechos reservados.
Desarrollado con JUCE. Fuentes: **Inter** (SIL Open Font License 1.1) y **JetBrains Mono**
(SIL Open Font License 1.1). VST es una marca registrada de Steinberg Media Technologies GmbH.
Audio Units y macOS son marcas de Apple Inc. Windows es una marca de Microsoft Corporation.
