# Investigacion: Frame Generation y Nuevos Modos de Escalado

Fecha: 2026-04-18

## 1. Estado actual del proyecto

El pipeline actual de Capturadora es:

1. `CaptureManager` captura por `OpenCV + CAP_MSMF`.
2. El frame entra como `cv::Mat` en RAM.
3. `VideoProcessor` aplica opcionalmente:
   - `GPUDenoiser`
   - `GPUUpscaler` (RTX Video Super Resolution)
   - `OpenCVUpscaler` (FSRCNN)
   - `cv::resize(..., INTER_LANCZOS4)` como AA / fallback
4. `AppWindow` presenta con `cv::imshow`.

Puntos clave encontrados en el codigo:

- La presentacion actual es OpenCV nativa, no un renderer DirectX. Ver `src/AppWindow.cpp`.
- El pipeline NVIDIA actual usa `NvCVImage` y `NvVFX_FX_VIDEO_SUPER_RES` con `CUDA`.
- `GPUUpscaler` y `GPUDenoiser` ya preservan un flujo corto GPU->GPU cuando se encadena `Denoise -> RTX`.
- La app no genera motion vectors, depth buffer, HUD-less color, ni maneja swap chain DX12.

## 2. Conclusiones rapidas

### Frame Generation con NVIDIA

Hay dos caminos muy distintos:

1. `DLSS Frame Generation / Multi Frame Generation`
   - No encaja de forma directa con la arquitectura actual.
   - Requiere una integracion tipo juego/renderer, normalmente via `Streamline`.
   - Exige `DirectX 12` para Frame Generation, ademas de motion vectors, depth, color sin HUD y control del present.
   - Hoy Capturadora no tiene nada de eso.

2. `NVIDIA Optical Flow SDK + FRUC`
   - Si encaja mucho mejor con el tipo de app que tenemos.
   - Esta pensado para interpolar frames de video o juego a partir de dos frames consecutivos.
   - Puede integrarse en una app `CUDA` o `DirectX`.
   - Es el candidato realista para agregar frame generation en Capturadora sin rehacer primero toda la app como renderer DX12.

### Nuevos escaladores

Los modos mas realistas para una primera expansion son:

1. `NVIDIA RTX VSR` adicionales:
   - Ya tenemos infraestructura.
   - Faltaria exponer mas modos del propio filtro: `Bicubic`, `Low`, `Medium`, `High`, `Ultra`, `HighBitrate_*`, `Deblur_*`.

2. Escaladores espaciales por shader o CPU/GPU livianos:
   - `Nearest`
   - `Bilinear`
   - `Sharpened Bilinear`
   - `Lanczos`
   - `NVIDIA Image Scaling (NIS)`
   - `AMD FSR 1`

3. Especializados:
   - `Anime4K`, pero solo vale la pena como camino aparte para contenido anime/2D.

## 3. Por que DLSS Frame Generation no es el primer paso correcto

Segun la documentacion oficial revisada:

- `Streamline` es la ruta recomendada por NVIDIA para integrar DLSS moderno.
- `DLSS Frame Generation` requiere `DirectX 12`.
- La checklist oficial menciona como entradas obligatorias motion vectors, depth, HUD-less color y UI buffer.
- Cuando FG esta activo, el present puede involucrar multiples colas y presents asincronos.

Eso choca con el estado actual del proyecto:

- `cv::imshow` no expone el control fino de `swap chain`, `command queue` ni `present`.
- La captura USB por OpenCV no produce motion vectors ni depth.
- La app no renderiza una escena 3D propia; recibe video 2D ya rasterizado.

Conclusion:

`DLSS Frame Generation / DLSS Multi Frame Generation` no es imposible a largo plazo, pero para este proyecto implicaria primero migrar la presentacion y buena parte del pipeline a `DirectX 12`, probablemente con textura compartida, compositor propio y una infraestructura de render muy distinta a la actual.

## 4. Camino recomendado para Frame Generation NVIDIA

### Opcion recomendada: Optical Flow SDK + FRUC

La documentacion oficial de `NVIDIA Optical Flow SDK` y `FRUC` encaja mucho mejor con Capturadora:

- FRUC genera un frame intermedio a partir de dos frames consecutivos.
- Usa el acelerador de optical flow dedicado del GPU y CUDA.
- Puede integrarse en aplicaciones `CUDA`.
- Acepta superficies `ARGB` y `NV12`.
- Esta pensado expresamente para video y frame rate up-conversion.

### Ventajas para Capturadora

- Reutiliza mejor el mundo actual `OpenCV -> CUDA`.
- No obliga a migrar primero a un renderer completo DX12.
- La RTX 5070 pertenece a la familia RTX 50, asi que por hardware deberia estar sobrada para este enfoque.
- Permite una fase inicial de prototipo sin destruir la arquitectura actual.

### Pero hay una advertencia importante

FRUC necesita, por definicion, al menos el frame anterior y el frame actual para generar uno intermedio.

Eso significa que:

- No podemos tener frame generation "gratis" sin mantener algo del frame previo.
- Debemos conservar exactamente un frame previo en GPU o CPU+GPU solo para interpolacion.
- Eso no es una cola ni buffering profundo, pero si es estado temporal minimo.

Esto sigue siendo compatible con la regla de baja latencia si:

- solo conservamos `1` frame previo;
- no encolamos multiples frames;
- no desacoplamos captura y display con buffers profundos;
- tratamos el frame generado como opcional y descartable si el tiempo de pipeline sube demasiado.

## 5. Dependencias necesarias para integrar Frame Generation con NVIDIA

### Para la ruta FRUC recomendada

Haria falta agregar o validar:

1. `NVIDIA Optical Flow SDK`
   - Header principal: `NvFRUC.h`
   - DLL: `NvFRUC.dll`
   - Muestras y wrappers de ejemplo

2. `CUDA Toolkit`
   - Ya existe en el proyecto, pero habria que validar version compatible con el SDK elegido.

3. `Driver NVIDIA reciente`
   - El FRUC guide indica soporte con drivers modernos en Windows.
   - El portal de descarga del Optical Flow SDK 5.0 menciona Windows driver `522.25+`.

4. Manejo de formatos
   - FRUC trabaja con `ARGB` o `NV12`.
   - El pipeline actual VFX usa `RGBA/BGRA` en GPU para VSR.
   - Habria que normalizar muy bien conversiones entre `BGR cv::Mat`, `RGBA/BGRA NvCVImage` y el formato de FRUC.

### Para una ruta DLSS FG a futuro

Ademas de drivers y SDKs, haria falta:

1. `NVIDIA Streamline SDK`
2. `DirectX 12 renderer`
3. Generacion de:
   - motion vectors
   - depth
   - color sin HUD
   - UI composition separada
4. Manejo de present y latencia tipo juego
5. Idealmente `NVIDIA Reflex`

Eso ya no es una ampliacion incremental: es una nueva capa de rendering para la app.

## 6. Como integraria FRUC en esta app sin romper el proyecto

### Nuevo modulo sugerido

`GPUFrameGenerator`

Responsabilidad:

- Mantener solo el frame previo.
- Convertir el frame actual a formato apto para FRUC.
- Pedir al SDK un frame interpolado.
- Decidir si mostrar:
  - frame real actual;
  - frame interpolado;
  - o ambos segun el modo operativo.

### Modos operativos posibles

1. `OFF`
2. `2x Preview`
   - 60 -> 120 visuales
   - muestra real/interpolado/real/interpolado
3. `Adaptive`
   - genera intermedio solo si el tiempo total del pipeline lo permite
4. `Latency First`
   - si el pipeline supera un umbral, omite el frame interpolado

### Donde insertarlo

Orden recomendado:

1. Captura
2. Denoise opcional
3. Upscale opcional
4. Frame generation
5. Display

Razon:

- generar sobre la imagen ya limpiada/escalada probablemente dara mejor calidad visual;
- ademas evita interpolar ruido MJPEG que luego se magnifica.

Riesgo:

- hacerlo despues de upscale aumenta costo por resolucion.

Alternativa:

- generar antes del upscale y luego escalar ambos frames.

Mi recomendacion real:

Probar ambas rutas y medir:

1. `Denoise -> FG -> Upscale`
2. `Denoise -> Upscale -> FG`

La mejor dependera de costo total y artefactos.

## 7. Escaladores que si vale la pena agregar

### A. Expandir lo que ya existe en NVIDIA RTX Video

Esto es lo mas facil y con mejor retorno.

El filtro `Video Super Resolution` oficial soporta:

- `0`: Bicubic
- `1-4`: VSR Low/Medium/High/Ultra
- `8-11`: Denoise
- `12-15`: Deblur
- `16-19`: HighBitrate Low/Medium/High/Ultra

Para Capturadora recomendaria exponer:

1. `RTX Bicubic`
2. `RTX VSR Low`
3. `RTX VSR Medium`
4. `RTX VSR High`
5. `RTX VSR Ultra`
6. `RTX HighBitrate Low`
7. `RTX HighBitrate Medium`
8. `RTX HighBitrate High`
9. `RTX HighBitrate Ultra`
10. `RTX Deblur Low/Medium/High/Ultra`

Observacion importante:

- `Deblur` y `Denoise` no escalan; su salida debe mantener la misma resolucion que la entrada.
- O sea, no son un reemplazo directo de VSR, sino otra etapa o un modo separado.

### B. Modos faciles y utiles estilo Lossless Scaling

Estos pueden implementarse sin dependencias pesadas:

1. `Nearest`
   - trivial con OpenCV
   - ideal para pixel art o UI muy nitida

2. `Bilinear`
   - trivial
   - mas suave

3. `Sharpened Bilinear`
   - bilinear + sharpen ligero
   - muy barato, buen modo general

4. `Lanczos 4`
   - ya esta casi presente como AA/fallback

5. `Bicubic`
   - barato y util como baseline

### C. Escaladores open source de mejor perfil tecnico

1. `NVIDIA Image Scaling (NIS)`
   - open source
   - espacial
   - muy buen candidato si en algun momento agregamos un renderer por shader

2. `AMD FSR 1`
   - open source
   - espacial
   - tambien es buen candidato para una ruta shader-based

Ambos son mucho mas razonables que intentar FSR 2/3 o DLSS SR dentro de la app actual, porque esos caminos exigen datos temporales y pipeline de render mas sofisticado.

### D. Anime4K

Anime4K existe y es open source, pero esta muy enfocado a anime/video 2D estilizado.

Para Capturadora:

- si el objetivo principal es gaming desde capturadora, no lo pondria en la primera fase;
- si quieres un "modo anime/2D/cartoon", entonces si puede tener sentido como modo especializado.

## 8. Roadmap recomendado

### Fase 1: Ganancia rapida y segura

1. Exponer mas modos del `GPUUpscaler` ya soportados por NVIDIA VFX:
   - VSR Low/Medium/High/Ultra
   - HighBitrate Low/Medium/High/Ultra
   - Bicubic
2. Separar conceptualmente:
   - `Escalado`
   - `Limpieza`
   - `Deblur`
3. Agregar escaladores ligeros:
   - Nearest
   - Bilinear
   - Bicubic
   - Sharpened Bilinear

### Fase 2: Prototipo de frame generation realista

1. Integrar `NVIDIA Optical Flow SDK`
2. Crear `GPUFrameGenerator`
3. Mantener solo 1 frame previo
4. Medir:
   - latencia real
   - costo GPU
   - artefactos en HUD/texto
   - comportamiento con 720p60 y 1080p60

### Fase 3: Escaladores avanzados adicionales

1. Evaluar `NIS`
2. Evaluar `FSR 1`
3. Evaluar `Anime4K` solo como modo especializado

### Fase 4: Camino largo y caro

Solo si de verdad queremos "algo tipo DLSS FG":

1. crear renderer `DX12`;
2. mover la presentacion fuera de `cv::imshow`;
3. componer UI por separado;
4. estudiar `Streamline + DLSS Frame Generation`.

## 9. Recomendacion final

La mejor ruta para Capturadora hoy es:

1. No empezar por `DLSS Frame Generation`.
2. Empezar por `NVIDIA Optical Flow SDK + FRUC` para frame generation.
3. En paralelo, ampliar de inmediato los modos de escalado ya disponibles dentro de `RTX Video / VFX`.
4. Luego sumar modos espaciales ligeros estilo Lossless Scaling.

Si tu objetivo es parecerse a Lossless Scaling sin destruir la arquitectura de baja latencia, esta combinacion es la mas sensata:

- `RTX VSR / HighBitrate / Deblur`
- `Nearest / Bilinear / Bicubic / Lanczos / Sharpened Bilinear`
- `FRUC` como frame generation NVIDIA

## 10. Fuentes revisadas

- README y documentacion local del proyecto:
  - `README.md`
  - `project-documentation.md`
  - `src/VideoProcessor.cpp`
  - `src/GPUUpscaler.cpp`
  - `src/GPUDenoiser.cpp`
  - `src/AppWindow.cpp`
  - `CMakeLists.txt`

- NVIDIA DLSS / Streamline:
  - https://developer.nvidia.com/rtx/streamline
  - https://developer.nvidia.com/rtx/streamline/get-started
  - https://developer.nvidia.com/rtx/dlss/get-started

- NVIDIA RTX Video / VFX:
  - https://developer.nvidia.com/rtx-video-sdk/getting-started
  - https://developer.nvidia.com/rtx-video-sdk
  - https://docs.nvidia.com/maxine/vfx/latest/Filters/VideoSuperResolution.html

- NVIDIA Optical Flow / FRUC:
  - https://developer.nvidia.com/optical-flow-sdk
  - https://developer.nvidia.com/opticalflow/download
  - https://docs.nvidia.com/video-technologies/optical-flow-sdk/nvfruc-programming-guide/index.html

- Otros escaladores:
  - https://developer.nvidia.com/rtx/image-scaling
  - https://gpuopen.com/fidelityfx-superresolution/
  - https://github.com/bloc97/Anime4K
