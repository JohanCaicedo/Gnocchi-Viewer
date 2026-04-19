# Gnocchi's Viewer

Visor de capturadoras en C++ para Windows con enfoque en baja latencia, limpieza de artefactos y escalado por IA.

## Que hace

- Captura video por `OpenCV + MSMF`.
- Prioriza un flujo de baja latencia sin colas ni buffering extra.
- Puede aplicar limpieza de artefactos con NVIDIA Video Effects.
- Puede escalar con `NVIDIA RTX VSR` o con `FSRCNN` como alternativa universal.
- Incluye puente de audio directo por `miniaudio`.
- Usa una ventana OpenCV con menu nativo Win32, sin frameworks GUI pesados.

## Flujo principal

1. Ingesta desde la capturadora por `CAP_MSMF`.
2. Frame en RAM.
3. Procesamiento opcional:
   - `NVIDIA Denoise`
   - `NVIDIA RTX VSR`
   - `FSRCNN`
   - `Lanczos 4`
4. Presentacion inmediata en ventana.

Cuando se usa `Denoise + RTX VSR`, el proyecto mantiene el encadenamiento entre filtros en VRAM antes de bajar el frame final para mostrarlo.

## Controles y menu

- `ESC`: alterna Zen Mode / fullscreen.
- `Q`: cierra la aplicacion.
- `Dispositivo`: selecciona la capturadora.
- `Ingesta (USB)`: cambia resolucion, FPS y MJPEG.
- `Procesamiento IA`: activa Denoise, FSRCNN, RTX VSR y resolucion objetivo.
- `Herramientas`: toma capturas de pantalla.
- `Opciones > Guardar Configuracion`: guarda el estado actual en `config.ini`.

## Requisitos

- Windows.
- Visual Studio Build Tools o Visual Studio con toolchain C++.
- CMake.
- vcpkg.
- CUDA Toolkit.
- Drivers NVIDIA compatibles para usar los filtros RTX.
- Carpeta `NVIDIA Video Effects` presente en la raiz del proyecto.

## Estructura esperada del SDK NVIDIA

El proyecto espera esta raiz:

```text
G:\dev\Capturadora\NVIDIA Video Effects
```

Y usa estas rutas en tiempo de ejecucion:

```text
NVIDIA Video Effects\bin
NVIDIA Video Effects\features\nvvfxvideosuperres\bin
NVIDIA Video Effects\models
```

## Notas de arquitectura

- No introducir buffering adicional en video o audio.
- `GPUUpscaler` trabaja sobre buffers RGBA.
- `GPUDenoiser` trabaja dentro del flujo NVIDIA VFX esperado por el proyecto.
- No remover la manipulacion dinamica de `PATH` en los modulos NVIDIA.
- Toda reserva GPU debe liberar sus recursos en `release()`.

## Estado actual

- Persistencia de configuracion funcionando.
- Seleccion de dispositivo de video restaurable.
- FSRCNN ajustado para respetar la resolucion objetivo final.
- Emparejamiento de audio mejorado usando el nombre real de la capturadora cuando esta disponible.

## Capturas

Las capturas se guardan en:

```text
G:\dev\Capturadora\capturas
```
