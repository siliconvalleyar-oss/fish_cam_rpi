# Registro de cambios

Todas las modificaciones notables del proyecto se documentan en este archivo.

El formato sigue [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/),
y el proyecto se adhiere a [Versionado Semántico](https://semver.org/lang/es/).

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
