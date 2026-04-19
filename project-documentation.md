# Especificacion Tecnica: Gnocchi's Viewer

## Objetivo

Construir un visor de capturadoras para Windows con latencia baja, usando una ruta de video directa y filtros opcionales de limpieza y escalado por IA.

## Stack actual

- `C++17`
- `OpenCV`
- `Media Foundation` a traves de `CAP_MSMF`
- `NVIDIA Video Effects SDK`
- `CUDA Runtime`
- `miniaudio`
- `Win32 API`

## Pipeline

1. Apertura de la capturadora con `CAP_MSMF`.
2. Configuracion de resolucion, FPS y MJPEG.
3. Captura con buffer size de `1`.
4. Procesamiento opcional:
   - Denoise NVIDIA
   - RTX VSR
   - FSRCNN
   - Lanczos 4
5. Render en ventana OpenCV.

## Modulos

### CaptureManager

- Detecta capturadoras disponibles.
- Usa nombres reales de Media Foundation cuando estan disponibles.
- Inicializa la captura con `CAP_MSMF`.
- Fuerza `MJPEG` cuando corresponde.

### VideoProcessor

- Orquesta `GPUDenoiser`, `GPUUpscaler` y `OpenCVUpscaler`.
- Mantiene el encadenamiento `Denoise -> RTX` con paso por VRAM.
- Para `FSRCNN`, aplica el modelo disponible y luego ajusta siempre al tamano objetivo final.

### GPUDenoiser

- Inicializa el efecto NVIDIA.
- Ajusta `PATH` para que Windows encuentre las DLLs del SDK.
- Libera todos los recursos GPU en `release()`.

### GPUUpscaler

- Inicializa RTX Video Super Resolution.
- Usa la raiz `NVIDIA Video Effects` del proyecto.
- Libera buffers y stream en `release()`.

### AudioManager

- Usa `miniaudio` en modo duplex.
- Intenta empatar el audio con la capturadora de video elegida.
- Si no encuentra coincidencia clara, usa un fallback heuristico.

### Application

- Controla el loop principal.
- Inyecta un menu Win32 sobre la ventana OpenCV.
- Guarda y restaura configuracion en `config.ini`.

## UX actual

- `ESC` alterna fullscreen.
- `Q` cierra la aplicacion.
- Menu superior para dispositivo, ingesta, IA, opciones y herramientas.
- Capturas en disco bajo `capturas\`.

## Restricciones importantes

- No introducir buffering adicional en la ruta de video ni de audio.
- Mantener la manipulacion dinamica de `PATH` para NVIDIA VFX.
- No romper la separacion de formatos y buffers esperada por el pipeline GPU.
- No sustituir `CAP_MSMF` en la ingesta USB del proyecto.
