# AI-Link Capture - Memory Notes

## Resumen

Proyecto Windows en C++ orientado a usar capturadoras como visor de baja latencia, con opciones de mejora por IA y una UI ligera basada en OpenCV + Win32.

## Componentes clave

- `CaptureManager`
  - Enumera video por Media Foundation.
  - Abre la captura por `CAP_MSMF`.
  - Mantiene `CAP_PROP_BUFFERSIZE = 1`.

- `VideoProcessor`
  - Coordina `GPUDenoiser`, `GPUUpscaler` y `OpenCVUpscaler`.
  - Tiene ruta `Denoise -> RTX` con paso intermedio en VRAM.
  - En `FSRCNN`, respeta siempre la resolucion objetivo final del menu.

- `GPUDenoiser` y `GPUUpscaler`
  - Dependen de la carpeta `NVIDIA Video Effects` en la raiz del proyecto.
  - Ajustan `PATH` antes de crear los efectos.
  - Liberan sus recursos explicitamente en `release()`.

- `AudioManager`
  - Usa `miniaudio`.
  - Intenta enlazar el input de audio a la capturadora de video elegida usando nombres reales.

- `Application`
  - Maneja menu Win32, fullscreen, screenshots, persistencia y ciclo principal.

## Persistencia

`config.ini` guarda:

- dispositivo visual y `hwIndex`
- resolucion de captura
- FPS
- resolucion objetivo
- `denoiseStrength`
- `aiType`
- flags de Denoise, AA, IA y MJPEG

## Estado reciente

- Corregido el guardado/restauracion de `AIType` y `denoiseStrength`.
- Corregido el apagado real de Denoise.
- Mejorado FSRCNN para respetar la resolucion objetivo.
- Mejorado el emparejamiento entre audio y video.
- Unificada la raiz esperada del SDK NVIDIA en el proyecto.

## Ruta SDK esperada

```text
G:\dev\Capturadora\NVIDIA Video Effects
```

## Riesgos a vigilar

- Reinicios innecesarios de filtros al cambiar opciones rapidamente.
- Warnings o cambios de API en dependencias externas.
- Modelos o DLLs faltantes en releases portables.
