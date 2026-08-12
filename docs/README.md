# fish_cam_rpi

Sistema de captura de imágenes para **Raspberry Pi 3/4/5** con la cámara
**OV5647** (lente gran angular de 130°), escrito en **C++17**.

Captura imágenes bajo demanda con marca de tiempo incrustada
(`YYYY-MM-DD HH:MM:SS`) y las guarda en **JPG o PNG con máxima calidad**
usando el patrón de archivo `fishcam_YYYYMMDD_HHMMSS.jpg`.

## Características

- Inicialización automática de la cámara OV5647 con reintentos (3 intentos).
- Configuración por archivo JSON (`config/camera_config.json`) o línea de comandos.
- Captura bajo demanda, en ráfaga (`--burst`) o timelapse (`--timelapse`).
- Overlay de timestamp con fondo semitransparente para máxima legibilidad.
- Salida JPG (calidad 1–100) o PNG (compresión 0–9), ambas con máxima calidad.
- Logging con 5 niveles (DEBUG, INFO, WARN, ERROR, CRITICAL) a consola y archivo.
- Manejo robusto de errores: timeout de captura (5 s), chequeo de espacio en disco
  (< 100 MB libres), fallback a JPG para formatos no soportados.
- API en C++ (RAII, hilos, singleton de logging) bajo licencia MIT.

## Estructura del proyecto

```
fish_cam_rpi/
├── bin/                → ejecutables compilados
├── obj/                → objetos (espejo de src/)
├── src/
│   ├── main.cpp        → punto de entrada y CLI
│   ├── camera/         → CameraManager (raspicam)
│   ├── config/         → ConfigManager (nlohmann/json)
│   ├── image/          → ImageProcessor (overlay y guardado)
│   ├── timestamp/      → TimestampManager (formato de fecha)
│   └── utils/          → Logger
├── include/            → cabeceras públicas (fish_cam.hpp)
├── config/             → camera_config.json
├── scripts/            → setup.sh, capture_demo.sh, test_camera.sh
├── script_tools/       → install_dependencies.sh
├── tests/              → pruebas unitarias y de integración
├── docs/               → documentación completa
├── logs/               → archivos de log
├── Makefile
└── .gitignore
```

## Inicio rápido

```bash
# 1. Instalar dependencias (una sola vez, requiere sudo y reinicio)
./script_tools/install_dependencies.sh
sudo reboot

# 2. Verificar que la cámara es detectada
./scripts/test_camera.sh

# 3. Compilar
make

# 4. Capturar una imagen
./bin/fish_cam_rpi --capture
```

La imagen se guarda en `captures/fishcam_YYYYMMDD_HHMMSS.jpg` con el
timestamp incrustado.

## Documentación

| Documento | Contenido |
|-----------|-----------|
| [INSTALL.md](INSTALL.md) | Instalación y dependencias |
| [USAGE.md](USAGE.md) | Guía de uso y ejemplos |
| [API.md](API.md) | Documentación técnica de la API |
| [ARCHITECTURE.md](ARCHITECTURE.md) | Arquitectura del sistema |
| [DEPLOYMENT.md](DEPLOYMENT.md) | Despliegue en producción |
| [CHANGELOG.md](CHANGELOG.md) | Registro de cambios |
| [CONTRIBUTING.md](CONTRIBUTING.md) | Guía para contribuir |
| [LICENSE.md](LICENSE.md) | Licencia (MIT) |

## Licencia

Distribuido bajo la licencia **MIT**. Ver [LICENSE.md](LICENSE.md).
