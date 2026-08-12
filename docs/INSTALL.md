# Instalación

Instrucciones para instalar y compilar **fish_cam_rpi** en Raspberry Pi OS
(Bullseye/Bookworm, 32 o 64 bits).

## 1. Requisitos

- Raspberry Pi 3/4/5 con Raspberry Pi OS instalado.
- Módulo de cámara **OV5647** (Pi Camera v1) con cable plano correctamente
  conectado.
- Conexión a internet para instalar dependencias.
- ~1 GB de espacio en disco.

## 2. Instalación de dependencias (recomendado)

Ejecuta el script de instalación:

```bash
./script_tools/install_dependencies.sh
```

El script realiza:

1. `apt-get update` e instalación de:
   - `build-essential`, `cmake`, `git`, `pkg-config`
   - `libopencv-dev` (procesamiento de imágenes)
   - `libjpeg-dev`, `libpng-dev`, `libtiff-dev` (codificadores)
   - `nlohmann-json3-dev` (configuración JSON)
   - `libraspberrypi-dev`, `libraspberrypi-bin` (cabeceras MMAL)
   - `libv4l-dev`, `v4l-utils` (V4L2)
2. Compilación e instalación de **raspicam** desde código fuente
   (usando el repositorio principal y, si falla, un fork compatible
   con Raspberry Pi OS reciente).
3. Habilitación de la **cámara legacy** en `/boot/config.txt`:
   - `camera_auto_detect=0`
   - `start_x=1`
   - `gpu_mem=128`
   - carga del módulo `bcm2835-v4l2`

> ⚠️ El script modifica `/boot/config.txt`. Si prefieres habilitar la cámara
> manualmente, usa `--skip-camera-config`:

```bash
./script_tools/install_dependencies.sh --skip-camera-config
```

### Habilitar la cámara manualmente (alternativa)

```bash
sudo raspi-config   # → Interface Options → I1 Legacy Camera → Enable
```

O edita `/boot/firmware/config.txt` (o `/boot/config.txt`) y añade:

```
camera_auto_detect=0
start_x=1
gpu_mem=128
```

## 3. Reiniciar

La configuración de la cámara requiere reinicio:

```bash
sudo reboot
```

## 4. Verificar la cámara

```bash
./scripts/test_camera.sh
```

Deberías ver una salida similar a:

```
supported=1 detected=1
RESULT: OV5647 camera OK
```

## 5. Compilar

```bash
make            # compila bin/fish_cam_rpi
make -j4        # compilación en paralelo (opcional)
```

Si `make` falla al localizar OpenCV/raspicam, revisa `make info` para ver la
configuración resuelta (usa `pkg-config`).

### Compilación manual de dependencias

Si no quieres usar los scripts:

```bash
sudo apt-get install -y build-essential cmake git pkg-config \
  libopencv-dev libjpeg-dev libpng-dev libtiff-dev nlohmann-json3-dev \
  libraspberrypi-dev libraspberrypi-bin libv4l-dev v4l-utils

git clone --depth 1 https://github.com/cedricve/raspicam.git ~/raspicam
cmake -S ~/raspicam -B ~/raspicam/build -DCMAKE_BUILD_TYPE=Release
cmake --build ~/raspicam/build -j$(nproc)
sudo cmake --install ~/raspicam/build
sudo ldconfig
```

> En Raspberry Pi OS Bookworm (12+) el repositorio principal de raspicam puede
> no compilar; usa el fork `https://github.com/fdlk/raspicam.git`.

## 6. Ejecutar las pruebas

```bash
make test                              # pruebas unitarias
./tests/integration_tests/test_integration.sh   # pruebas de integración
```

## Dependencias finales

| Dependencia        | Propósito                          |
|--------------------|------------------------------------|
| `libopencv-dev`    | procesamiento y guardado de imagen |
| `raspicam`         | acceso a la cámara (MMAL)          |
| `nlohmann-json3-dev` | configuración JSON               |
| `libjpeg-dev`/`libpng-dev` | codificadores               |
| `libraspberrypi-dev` | cabeceras MMAL del Pi            |
| `build-essential`/`cmake` | compilación                  |
