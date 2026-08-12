# Registro de cambios

Todas las modificaciones notables del proyecto se documentan en este archivo.

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y el proyecto se adhiere a [Versionado Semántico](https://semver.org/lang/es/).

## [0.1.10] - 2026-08-12

### Corregido
- Backend OpenCV en Bookworm+ con cámara CSI: capturaba frames verdes
  (crominancia nula). OpenCV 4.6 lee el buffer NV21/YUYV de la capa
  v4l2-compat de libcamera sin crominancia (solo plano Y), mientras que la
  cámara y libcamera producen imagen correcta (verificado con rpicam-still).
  El backend ahora intenta primero una pipeline GStreamer `libcamerasrc` (usa
  el ISP de libcamera directamente: color correcto, sin LD_PRELOAD) y solo cae
  al backend V4L2 si el plugin `gstreamer1.0-libcamera` no está instalado.
- Se descartan los primeros frames negros de arranque de libcamerasrc para que
  la primera captura real tenga contenido.

### Añadido
- `install_dependencies.sh` instala `gstreamer1.0-libcamera` en Bookworm+ y
  actualiza las instrucciones de ejecución (GStreamer preferido, LD_PRELOAD
  como fallback). `docs/INSTALL.md` también se actualizó.

## [0.1.9] - 2026-08-12

### Corregido
- Backend raspicam (`CameraManager`): se usaba una API inexistente de
  `RaspiCam_Cv` (`setFrameRate`, `setISO`, `setEncoding`, `open(cv::Size)`,
  ...), lo que impedía compilar en Raspberry Pi OS 32-bit con la pila legacy.
  Ahora se usa la API real (`set(CAP_PROP_FRAME_WIDTH/HEIGHT/FPS/...)`,
  `setRotation`, `open()` sin argumentos). La exposición queda en automático
  (RaspiCam_Cv no expone modo manual) y `sharpness`/`quality`/`encoding`/
  `preview` se gestionan aguas abajo al codificar.

## [0.1.8] - 2026-08-12

### Corregido
- Backend OpenCV V4L2 en Bookworm+ (libcamera v4l2-compat): ya no se fuerza
  MJPG (`CAP_PROP_FOURCC`), que la capa de compatibilidad no soporta y hacía
  que el stream nunca arrancara colgando `read()` indefinidamente. Ahora se usa
  NV21 (y YUYV como fallback), que sí expone la capa v4l2-compat.
- `CameraManager::Capture()` con backend OpenCV: `VideoCapture::read()` en V4L2
  se bloquea sin límite (`CAP_PROP_READ_TIMEOUT_MSEC` solo aplica a FFmpeg).
  Ahora la lectura corre en un hilo auxiliar y se respeta `capture_timeout_ms`.

### Añadido
- `script_tools/kill_fish_cam.sh`: lista los procesos `fish_cam_rpi` en un menú
  y permite matarlos uno a uno o todos (`--all`), p. ej. para liberar
  `/dev/video0` cuando una captura se queda colgada.

## [0.1.7] - 2026-08-12

### Corregido
- `script_tools/install_dependencies.sh`: en Raspberry Pi OS **Bookworm+**
  instala `libcamera-v4l2` y `libcamera-apps` (la pila de cámara moderna). Al
  final avisa del comando de ejecución correcto con la capa de compatibilidad
  V4L2 (`LD_PRELOAD=.../v4l2-compat.so`), sin la cual OpenCV no puede abrir
  `/dev/video0` (que en Bookworm no existe de forma nativa).
- `script_tools/check_camera.sh`: detecta nodos `/dev/video*` presentes pero
  sin `/dev/video0`, avisando de que hace falta la capa v4l2-compat.

## [0.1.6] - 2026-08-12

### Añadido
- `script_tools/check_camera.sh`: script de diagnóstico de la cámara
  (dispositivos `/dev/video*`, estado de la pila legacy/libcamera, config.txt
  y captura real con `bin/fish_cam_rpi`). Referenciado por
  `install_dependencies.sh` y `src/main.cpp` pero hasta ahora inexistente.

### Corregido
- Referencias rotas a `scripts/test_camera.sh` en README, INSTALL, DEPLOYMENT
  y USAGE; ahora apuntan al script real.

## [0.1.5] - 2026-08-11

### Corregido
- `script_tools/install_dependencies.sh`: ya no intenta compilar raspicam en
  Raspberry Pi OS **Bookworm+** (la pila legacy MMAL no existe ahí); detecta
  la pila (`/opt/vc` o cabeceras MMAL) y, si no está, lo omite y avisa que se
  usará el backend OpenCV V4L2. Eliminado el fallback al fork `fdlk/raspicam`
  (repositorio inexistente que colgaba el `git clone` pidiendo credenciales);
  `GIT_TERMINAL_PROMPT=0` evita que cualquier clone se quede esperando input.
- `enable_legacy_camera()` y `verify()` solo tocan la configuración legacy
  cuando la pila existe; en Bookworm se conserva `camera_auto_detect`.

## [0.1.4] - 2026-08-11

### Corregido
- Ruta de la capa de compatibilidad V4L2 de libcamera en `INSTALL.md` y en el
  comentario de `CameraManager.hpp`: está en
  `/usr/libexec/<multiarch>/libcamera/v4l2-compat.so` (paquete `libcamera-v4l2`).

### Añadido
- Sección de solución de problemas en `docs/INSTALL.md`: "No cameras
  available!" y cómo forzar `dtoverlay=ov5647` (módulos de terceros).

## [0.1.3] - 2026-08-11

### Añadido
- **Backend de cámara OpenCV V4L2** (`FISH_CAM_USE_OPENCV_BACKEND`): en
  Raspberry Pi OS Bookworm+ raspicam ya no está disponible (la pila legacy
  MMAL fue eliminada); el Makefile lo detecta y compila `CameraManager` sobre
  `cv::VideoCapture`/`/dev/video0`. El backend raspicam sigue siendo el
  predeterminado cuando está presente.

### Corregido
- `Makefile`: ya no fuerza `-lraspicam` cuando la librería no existe; detecta
  raspicam vía `pkg-config` o `/usr/local/lib` y añade `-lopencv_videoio` al
  fallback de OpenCV.

## [0.1.2] - 2026-08-11

### Corregido
- `src/main.cpp`: faltaba `using namespace fishcam;`, lo que rompía la
  compilación (errores "`AppSettings` is of non-class type 'const int'",
  "`Logger` has not been declared", etc.).
- `CameraSettings`: añadido el campo `frame_rate` (lo esperaban la CLI `--fps`
  y `camera_config.json`, pero no existía en la configuración). Ahora se carga
  y guarda correctamente.

## [0.1.1] - 2026-08-11

### Añadido
- Política de versionado: cada release incrementa el patch en **0.0.1**
  (`v0.1.0 → v0.1.1 → ...`) y cada push/release lleva su tag `vX.Y.Z`.
- `scripts/release.sh`: automatiza el bump de versión, el commit y el tag.
- Archivo `VERSION` y compilación con `-DFISH_CAM_VERSION` (Makefile).
- Objetivo `make version`.

## [0.1.0] - 2026-08-11

### Añadido
- Estructura completa del proyecto (src/, include/, config/, scripts/,
  script_tools/, tests/, docs/).
- `CameraManager`: inicialización de la OV5647 con 3 reintentos, timeout de
  captura de 5 s, captura síncrona/asíncrona y ráfaga (`CaptureBurst`).
- `ImageProcessor`: overlay de timestamp con fondo y guardado JPG/PNG con
  máxima calidad.
- `TimestampManager`: formatos `YYYY-MM-DD HH:MM:SS` y
  `fishcam_YYYYMMDD_HHMMSS.<ext>`.
- `ConfigManager`: carga de `config/camera_config.json` con nlohmann/json.
- `Logger`: singleton seguro para hilos con niveles DEBUG/INFO/WARN/ERROR/CRITICAL.
- CLI: `--capture`, `--burst`, `--timelapse`, `--count`, `--output`,
  `--config`, ajustes de cámara, `--demo`, `--verbose`, `--help`.
- `script_tools/install_dependencies.sh`: instalación automática de
  dependencias y habilitación de la cámara legacy.
- `scripts/setup.sh`, `scripts/capture_demo.sh`, `scripts/test_camera.sh`.
- Pruebas unitarias (`make test`) y de integración.
- Makefile con objetivos `all`, `run`, `demo`, `test`, `install`,
  `uninstall`, `info`, `clean`, `distclean`.
- Documentación completa en `docs/`.

## [No publicado]

### Planeado
- Soporte de libcamera (moderna) como alternativa a raspicam.
- Interfaz HTTP/API REST para disparar capturas remotamente.
- Pila de imágenes (ring buffer) para el modo ráfaga.
- Archivos de despliegue systemd de ejemplo (ver DEPLOYMENT.md).

## Política de versionado

Cada release incrementa la versión **patch en 0.0.1** (`v0.1.0 → v0.1.1 → ...`)
y lleva su tag anotado `vX.Y.Z`. La publicación se automatiza con
`scripts/release.sh` (ver docs/CONTRIBUTING.md).
