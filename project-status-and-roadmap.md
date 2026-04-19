# Estado Actual y Roadmap de Capturadora

Fecha: 2026-04-18

## Documentos relacionados

- [README.md](G:\dev\Capturadora\README.md)
- [project-documentation.md](G:\dev\Capturadora\project-documentation.md)
- [research-frame-generation-and-scaling.md](G:\dev\Capturadora\research-frame-generation-and-scaling.md)

## Resumen ejecutivo

Capturadora ya tiene un pipeline funcional de baja latencia para capturar, limpiar, escalar y mostrar video en Windows usando `OpenCV + MSMF + NVIDIA Video Effects + CUDA + Win32`.

En este ciclo de trabajo también se avanzó en dos líneas nuevas:

1. Base de `Frame Generation` con `NVIDIA Optical Flow SDK + FRUC`.
2. Nuevos modos de escalado, incluyendo escaladores espaciales ligeros y una primera integración de `Anime4KCPP`.

## Qué ya estaba funcionando antes de esta fase

- Captura por `CAP_MSMF` forzando `MJPEG` para mantener 60 FPS sobre USB.
- Pipeline de procesamiento con:
  - `NVIDIA Denoise / Artifact Reduction`
  - `NVIDIA RTX VSR`
  - `FSRCNN`
  - `Lanczos 4`
- Encadenamiento `Denoise -> RTX` manteniendo paso por VRAM.
- Audio bridge por `miniaudio`.
- Menú nativo Win32.
- Persistencia en `config.ini`.
- Zen mode / fullscreen.

## Qué se agregó en Fase 1

### 1. Base de FRUC

Se integró el `NVIDIA Optical Flow SDK` al proyecto para dejar lista la base de `Frame Generation`.

Archivos principales:

- [GPUFrameGenerator.hpp](G:\dev\Capturadora\include\GPUFrameGenerator.hpp)
- [GPUFrameGenerator.cpp](G:\dev\Capturadora\src\GPUFrameGenerator.cpp)
- [CMakeLists.txt](G:\dev\Capturadora\CMakeLists.txt)

Estado actual:

- La DLL `NvOFFRUC.dll` ya se carga desde el proyecto.
- El build ya empaqueta el runtime necesario.
- La app ya sabe si la base FRUC está disponible.
- Aún no se están interpolando fotogramas reales con FRUC.

### 2. Toggle de Frame Generation Preview

Se agregó una ruta de UX para `Frame Generation`, todavía como preview.

Archivos principales:

- [Application.hpp](G:\dev\Capturadora\include\Application.hpp)
- [Application.cpp](G:\dev\Capturadora\src\Application.cpp)
- [ConfigManager.hpp](G:\dev\Capturadora\include\ConfigManager.hpp)
- [ConfigManager.cpp](G:\dev\Capturadora\src\ConfigManager.cpp)

Estado actual:

- El menú permite activar y desactivar `Frame Generation (Preview x2)`.
- El estado se guarda en `config.ini`.
- El viewer de FPS puede reflejar el FPS de salida esperado cuando FG Preview está activo.
- Esto es solo preview visual de estado, no FRUC real todavía.

### 3. Viewer de FPS movido a Herramientas

Se movió el overlay de FPS para que sea opcional.

Estado actual:

- `Herramientas > Viewer de FPS`
- Se puede prender y apagar.
- También queda persistido en `config.ini`.

### 4. Nuevos escaladores espaciales ligeros

Se agregaron modos ligeros estilo Lossless Scaling.

Archivos principales:

- [OpenCVUpscaler.hpp](G:\dev\Capturadora\include\OpenCVUpscaler.hpp)
- [OpenCVUpscaler.cpp](G:\dev\Capturadora\src\OpenCVUpscaler.cpp)
- [VideoProcessor.hpp](G:\dev\Capturadora\include\VideoProcessor.hpp)
- [VideoProcessor.cpp](G:\dev\Capturadora\src\VideoProcessor.cpp)
- [Application.cpp](G:\dev\Capturadora\src\Application.cpp)

Modos agregados:

- `Nearest`
- `Bilinear`
- `Bicubic`
- `Lanczos 4`
- `Sharpened Bilinear`

## Qué se agregó en Fase 2

### Primera integración de Anime4K

Se integró `Anime4KCPP` como dependencia local del proyecto.

Ruta:

- [third_party/Anime4KCPP](G:\dev\Capturadora\third_party\Anime4KCPP)

Archivos principales de integración:

- [Anime4KUpscaler.hpp](G:\dev\Capturadora\include\Anime4KUpscaler.hpp)
- [Anime4KUpscaler.cpp](G:\dev\Capturadora\src\Anime4KUpscaler.cpp)
- [VideoProcessor.cpp](G:\dev\Capturadora\src\VideoProcessor.cpp)
- [Application.cpp](G:\dev\Capturadora\src\Application.cpp)
- [CMakeLists.txt](G:\dev\Capturadora\CMakeLists.txt)

Estado actual:

- Ya existe un modo `Anime4K` en el menú.
- Está conectado al pipeline actual de la app.
- Se integró usando `Anime4KCPP core` únicamente.
- No se usa el módulo de video ni `FFmpeg`.
- La ruta actual usa `CPU` con el modelo `ACNet-HDN0`.

## Estado actual del menú

### Procesamiento IA

- `NVIDIA Denoise`
- `Frame Generation (Preview x2)`
- `AA Lanczos 4`
- `NVIDIA RTX VSR`
- `Universal FSRCNN`
- `Nearest`
- `Bilinear`
- `Bicubic`
- `Lanczos 4`
- `Sharpened Bilinear`
- `Anime4K`

### Herramientas

- `Tomar Captura`
- `Viewer de FPS`

## Estado técnico actual por módulo

### Captura

- Sigue usando `OpenCV + CAP_MSMF`.
- Sigue priorizando `MJPEG`.
- No se ha alterado la arquitectura base de baja latencia.

### Procesamiento

- `VideoProcessor` ahora puede orquestar:
  - `RTX VSR`
  - `FSRCNN`
  - escaladores espaciales ligeros
  - `Anime4K`
  - base de `Frame Generation`

### NVIDIA

- `GPUUpscaler` y `GPUDenoiser` siguen funcionales.
- `GPUFrameGenerator` ya carga `NvOFFRUC.dll`, pero no procesa frames todavía.

### Configuración

`config.ini` ya persiste:

- dispositivo
- resolución de captura
- resolución objetivo
- modo IA
- denoise
- frame generation preview
- viewer de FPS
- AA
- formato MJPEG

## Qué falta o qué sigue pendiente

### 1. FRUC real

Todavía no existe interpolación de fotogramas real en tiempo de ejecución.

Falta implementar:

- registro de recursos CUDA para FRUC
- paso de frame previo + frame actual
- generación real del frame intermedio
- estrategia de presentación del frame generado
- medición de costo y latencia

### 2. Optimizar Anime4K

La integración actual sirve como base, pero todavía hay trabajo para dejarla bien afinada.

Pendientes recomendados:

- evaluar si `ACNet`, `ARNet` o `ArtCNN` da mejor resultado en capturadora
- medir rendimiento real a `720p60` y `1080p60`
- evaluar si conviene una ruta `CUDA` de Anime4KCPP en vez de CPU
- agregar opciones de calidad / modelo en el menú

### 3. Más modos NVIDIA

Todavía falta exponer varios modos ya soportados por `RTX Video / VFX`, por ejemplo:

- `RTX Bicubic`
- `VSR Low / Medium / High / Ultra`
- `HighBitrate Low / Medium / High / Ultra`
- `Deblur`

### 4. Telemetría más clara

Conviene mejorar el overlay y logs para distinguir:

- FPS base
- FPS de salida
- costo por etapa
- si la app está usando RTX, FSRCNN, Anime4K o espacial
- si FG preview es solo estimado o real

## Riesgos y observaciones

### FRUC

- Requiere manejar al menos un frame previo.
- Eso debe hacerse sin romper la filosofía de baja latencia del proyecto.

### Anime4K

- La implementación actual usa CPU.
- Podría ser pesada para ciertas resoluciones o escenas.
- Anime4K está optimizado sobre todo para anime/2D, no necesariamente para todo tipo de señal de capturadora.

### Dependencias de terceros

- `Anime4KCPP` compila y enlaza, pero genera warnings internos del tercero.
- El `Optical Flow SDK` ya está en el repo y funcional para la base de integración.

## Recomendación práctica para la siguiente etapa

Orden sugerido:

1. Medir el rendimiento real del modo `Anime4K`.
2. Añadir selector de perfil/modelo para `Anime4K`.
3. Exponer más modos de `RTX Video`.
4. Solo después, avanzar con `FRUC` real.

## Archivos clave del estado actual

- [CMakeLists.txt](G:\dev\Capturadora\CMakeLists.txt)
- [Application.cpp](G:\dev\Capturadora\src\Application.cpp)
- [VideoProcessor.cpp](G:\dev\Capturadora\src\VideoProcessor.cpp)
- [OpenCVUpscaler.cpp](G:\dev\Capturadora\src\OpenCVUpscaler.cpp)
- [Anime4KUpscaler.cpp](G:\dev\Capturadora\src\Anime4KUpscaler.cpp)
- [GPUFrameGenerator.cpp](G:\dev\Capturadora\src\GPUFrameGenerator.cpp)
- [ConfigManager.cpp](G:\dev\Capturadora\src\ConfigManager.cpp)

## Estado final de este momento

Capturadora ya no es solo un viewer con `RTX VSR + FSRCNN`.

Ahora mismo el proyecto ya tiene:

- base de `Frame Generation` con NVIDIA Optical Flow
- preview de FG en la UX
- viewer de FPS opcional
- varios escaladores espaciales ligeros
- integración inicial de `Anime4K`

El siguiente salto importante no es agregar más estructura, sino validar calidad, rendimiento y decidir qué modo merece quedarse como opción estable para uso diario.
