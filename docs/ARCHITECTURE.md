# Arquitectura del sistema

## Visión general

`fish_cam_rpi` es una aplicación de línea de comandos en C++17 que captura
imágenes de una cámara **OV5647** en una Raspberry Pi. Sigue el principio de
**responsabilidad única (SRP)**: cada módulo tiene una sola tarea y expone una
interfaz clara.

```
+------------------+       +------------------+       +-------------------+
|      main        |       |  CameraManager   |       |  ImageProcessor   |
| (CLI + orquest.) | --->  |  (raspicam/MMAL) | --->  | (OpenCV: overlay) |
+------------------+       +------------------+       +-------------------+
      |   |                          |                         |
      |   +--> ConfigManager         +--> Logger              +--> Guardado
      |        (JSON/nlohmann)            (consola+archivo)        JPG/PNG 100%
      +--> TimestampManager
            (fechas/nombres)
```

## Flujo de datos (captura única)

```
1. Parseo de CLI (--burst/--timelapse/...)
2. Carga de config/camera_config.json  →  ConfigManager
3. Aplicación de overrides de CLI
4. Configuración del Logger
5. Detección de cámaras (DetectCameras)
6. CameraManager::Initialize()  (3 reintentos, timeout 5 s)
7. CameraManager::Capture(frame)  → cv::Mat BGR
8. ImageProcessor::OverlayTimestamp(frame, "YYYY-MM-DD HH:MM:SS")
9. ImageProcessor::Save(frame, captures/fishcam_YYYYMMDD_HHMMSS.jpg)
10. Logging del resultado (tamaño, resolución, tiempos)
11. CameraManager::Shutdown()  (RAII en destructor)
```

## Módulos

### `src/main.cpp`
- Parsea la CLI, orquesta los modos (single / burst / timelapse / demo),
  carga configuración y reporta resultados.
- Registra manejadores de `SIGINT`/`SIGTERM` para detener timelapse con Ctrl-C.
- Comprueba espacio en disco (< 100 MB libres avisa) y eleva la prioridad
  del proceso (`setpriority -10`, si hay permisos).

### `src/camera/CameraManager.{hpp,cpp}`
- Envuelve `raspicam::RaspiCam_Cv`.
- **RAII**: `Shutdown()` se llama desde el destructor.
- **Seguridad entre hilos**: un `std::mutex` serializa `Capture`,
  `CaptureBurst` y `Shutdown`.
- `CaptureAsync()` lanza un hilo de trabajo (captura desacoplada del proceso).
- `CaptureBurst()` preasigna un vector de `cv::Mat` como buffer de imágenes.
- Configuración del sensor: resolución, FPS, calidad, ISO, exposición,
  brillo, contraste, saturación, nitidez, rotación y flips.

### `src/image/ImageProcessor.{hpp,cpp}`
- Dibuja el timestamp con fondo negro (legibilidad sobre cualquier escena).
- Guarda con **máxima calidad**: JPEG `IMWRITE_JPEG_QUALITY=100`,
  PNG `IMWRITE_PNG_COMPRESSION=0`.
- Crea recursivamente los directorios de salida.

### `src/timestamp/TimestampManager.{hpp,cpp}`
- Formato overlay: `YYYY-MM-DD HH:MM:SS`.
- Formato archivo: `fishcam_YYYYMMDD_HHMMSS.<ext>`.
- Usa `localtime_r` (seguro entre hilos) y `std::put_time`.

### `src/config/ConfigManager.{hpp,cpp}`
- Lee `camera_config.json` con **nlohmann/json**; las claves ausentes
  conservan los valores por defecto.
- Los valores de CLI sobrescriben los del JSON.

### `src/utils/Logger.{hpp,cpp}`
- Singleton seguro para hilos (mutex + `localtime_r`).
- Niveles: DEBUG, INFO, WARN, ERROR, CRITICAL.
- Salida simultánea a consola y archivo (`kBoth`).

## Hilos

| Hilo | Origen | Uso |
|------|--------|-----|
| Principal | `main` | CLI, orquestación, guardado |
| Worker (opcional) | `CaptureAsync` | captura asíncrona |
| Worker (detección) | `DetectCameras` (popen) | enumeración de dispositivos |

`CaptureAsync` usa un hilo `detach()`: no debe destruirse el `CameraManager`
mientras haya capturas asíncronas en vuelo (ver `API.md`).

## Optimizaciones aplicadas

- **Buffer de imágenes**: `CaptureBurst` preasigna el vector de frames.
- **Captura asíncrona**: `CaptureAsync` permite captura desacoplada.
- **Compresión en tiempo real**: la codificación JPG/PNG ocurre en el
  guardado con calidad máxima.
- **Prioridad de proceso**: `setpriority(…, -10)` con `sudo`.
- **Overlay barato**: `putText` + rectángulo relleno en un solo pase.

## Manejo de errores

| Condición | Comportamiento |
|-----------|----------------|
| Cámara no detectada | 3 intentos de `open()` con backoff; mensaje CRITICAL y código de salida 1. |
| Timeout de captura | `grab()` con timeout de 5 s (configurable). |
| Formato no soportado | Fallback a JPG con advertencia. |
| Espacio en disco < 100 MB | Advertencia (no bloquea). |
| Archivo existente y `overwrite=false` | Sufijo `_1`, `_2`, ... |

## Estructura de directorios

```
src/
├── camera/    CameraManager  (hardware: raspicam)
├── config/    ConfigManager  (persistencia: JSON)
├── image/     ImageProcessor (procesamiento: OpenCV)
├── timestamp/ TimestampManager (utilidades de fecha)
└── utils/     Logger         (infraestructura)
obj/           espejo de src/ con los .o y .d
include/       cabecera pública agregada (fish_cam.hpp)
tests/         unit_tests (assert) + integration_tests (shell)
```

## Decisiones técnicas

- **C++17** por `std::filesystem`, `std::optional`, lambdas genéricas.
- **raspicam** para la cámara: API sencilla sobre MMAL; requiere la pila
  legacy de la cámara.
- **OpenCV** para el overlay y codificación (ampliamente disponible en apt).
- **nlohmann/json**: cabecera-solo, sin dependencias de compilación.
- Patrones: Singleton (`Logger`), RAII (`CameraManager`), Factoría de
  configuración (`ConfigManager`), utilidad estática (`TimestampManager`).
