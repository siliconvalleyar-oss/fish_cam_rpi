# Guía de uso

## Sinopsis

```bash
./bin/fish_cam_rpi [opciones]
```

## Modos de captura

| Opción              | Descripción                                   |
|---------------------|-----------------------------------------------|
| `--capture`         | Captura única bajo demanda (por defecto).     |
| `--burst <N>`       | Ráfaga de N imágenes consecutivas.            |
| `--timelapse <S>`   | Captura una imagen cada S segundos.           |
| `--count <N>`       | Límite de imágenes en timelapse (0 = infinito). |
| `--output <archivo>`| Nombre de archivo explícito para la captura.  |

## Configuración de la cámara

| Opción                  | Descripción                                 |
|-------------------------|---------------------------------------------|
| `--config <archivo>`    | Cargar configuración JSON.                  |
| `--width <px>`          | Ancho (por defecto 1920).                   |
| `--height <px>`         | Alto (por defecto 1080).                    |
| `--fps <n>`             | Fotogramas por segundo (por defecto 30).    |
| `--format <jpg\|png>`   | Formato de salida.                          |
| `--quality <1..100>`    | Calidad (por defecto 100 = máxima).         |
| `--iso <auto\|número>`  | Sensibilidad ISO.                           |
| `--exposure <modo>`     | `auto`, `off`, `night`, `backlight`, ...    |
| `--rotation <deg>`      | Rotación (múltiplos de 90).                 |
| `--brightness <0..100>` | Brillo (por defecto 50).                    |
| `--contrast <0..100>`   | Contraste (por defecto 50).                 |
| `--saturation <0..100>` | Saturación (por defecto 50).                |
| `--sharpness <-100..100>` | Nitidez (por defecto 50).                 |
| `--preview`             | Ventana de vista previa en vivo.            |

## Salida y logging

| Opción              | Descripción                                      |
|---------------------|--------------------------------------------------|
| `--output-dir <dir>`| Directorio de salida (por defecto `./captures`). |
| `--no-overlay`      | No incrustar el timestamp en la imagen.          |
| `--log-file <path>` | Archivo de log (por defecto `./logs/camera.log`).|
| `--verbose`         | Logging en nivel DEBUG.                          |

## Otras

| Opción          | Descripción                                      |
|-----------------|--------------------------------------------------|
| `-d, --demo`    | Modo demo sin cámara (genera 3 imágenes sintéticas). |
| `-h, --help`    | Muestra la ayuda y termina.                      |

## Ejemplos

```bash
# Captura única con configuración por defecto
./bin/fish_cam_rpi --capture

# Captura con nombre explícito en PNG
./bin/fish_cam_rpi --capture --output foto_peces.png --format png

# Ráfaga de 10 imágenes a 2592x1944
./bin/fish_cam_rpi --burst 10 --width 2592 --height 1944 --quality 100

# Timelapse: una foto cada 60 segundos, durante 30 fotos
./bin/fish_cam_rpi --timelapse 60 --count 30

# Timelapse indefinido (detener con Ctrl-C)
./bin/fish_cam_rpi --timelapse 300

# Usar un archivo de configuración personalizado
./bin/fish_cam_rpi --config config/camera_config.json --burst 5

# Modo demo (sin cámara, para validar el pipeline)
./bin/fish_cam_rpi --demo

# Vista previa en vivo y captura a máxima calidad
./bin/fish_cam_rpi --preview --capture
```

## Archivo de configuración

`config/camera_config.json`:

```json
{
  "camera": {
    "resolution": { "width": 1920, "height": 1080 },
    "format": "jpg",
    "quality": 100,
    "iso": "auto",
    "brightness": 50,
    "contrast": 50,
    "saturation": 50,
    "sharpness": 50,
    "exposure": "auto",
    "rotation": 0
  },
  "output": {
    "directory": "./captures",
    "filename_pattern": "fishcam_%Y%m%d_%H%M%S",
    "max_size_mb": 10,
    "overwrite": false,
    "overlay": true
  },
  "logging": {
    "level": "info",
    "file": "./logs/camera.log",
    "max_size_mb": 5,
    "backup_count": 3
  }
}
```

Los valores de la línea de comandos tienen **prioridad** sobre los del JSON.

## Salida esperada

```
[2026-08-11 22:40:00] [INFO] fish_cam_rpi 0.1.0 (build date Aug 11 2026)
[2026-08-11 22:40:00] [INFO] Config: 1920x1080 @ 30 fps, jpg 100%
[2026-08-11 22:40:00] [INFO] Camera opened successfully (OV5647, 130-degree lens)
[2026-08-11 22:40:01] [INFO] Captura exitosa: captures/fishcam_20260811_224001.jpg (2.3 MB), 1920x1080, captura 850 ms, overlay 12 ms, total 900 ms, timestamp: añadido
[2026-08-11 22:40:01] [INFO] fish_cam_rpi terminated (exit 0)
```

## Solución de problemas

| Problema | Solución |
|----------|----------|
| `Camera initialization failed` | Verifica el cable de la cámara, ejecuta `script_tools/check_camera.sh` y la cámara legacy (INSTALL.md). |
| `grab() failed / capture timeout` | Baja la resolución o sube la iluminación; comprueba el timeout. |
| Imagen demasiado grande | Reduce `--width/--height` o usa JPG con calidad menor. |
| `Low disk space` | Libera espacio; el programa avisa con < 100 MB libres. |
| Log no se escribe | Comprueba permisos del directorio de logs. |
