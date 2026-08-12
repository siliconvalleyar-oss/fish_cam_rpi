# Despliegue

Guía para desplegar `fish_cam_rpi` en producción en una Raspberry Pi.

## 1. Preparación del hardware

- Raspberry Pi 3/4/5 con Raspberry Pi OS (Bullseye o Bookworm).
- Cámara OV5647 (Pi Camera v1) conectada con el cable plano orientado hacia
  el conector.
- Alimentación adecuada (las caídas de tensión pueden reiniciar la cámara).
- Si el cable es largo (> 30 cm), usa cables blindados o repetidores.

## 2. Instalación

```bash
git clone <url-del-repositorio> fish_cam_rpi
cd fish_cam_rpi
./script_tools/install_dependencies.sh
sudo reboot
./scripts/test_camera.sh       # debe reportar: RESULT: OV5647 camera OK
make
```

## 3. Configuración de producción

Crea un archivo de configuración dedicado, p. ej. `config/prod.json`:

```json
{
  "camera": {
    "resolution": { "width": 2592, "height": 1944 },
    "format": "jpg",
    "quality": 100,
    "iso": "auto",
    "exposure": "auto",
    "rotation": 0
  },
  "output": {
    "directory": "/srv/fish_cam/captures",
    "filename_pattern": "fishcam_%Y%m%d_%H%M%S",
    "max_size_mb": 10,
    "overwrite": false,
    "overlay": true
  },
  "logging": {
    "level": "info",
    "file": "/srv/fish_cam/logs/camera.log",
    "max_size_mb": 5,
    "backup_count": 3
  }
}
```

Crea los directorios y permisos:

```bash
sudo mkdir -p /srv/fish_cam/captures /srv/fish_cam/logs
sudo chown -R pi:pi /srv/fish_cam
```

## 4. Servicio systemd (timelapse permanente)

`/etc/systemd/system/fish_cam.service`:

```ini
[Unit]
Description=fish_cam_rpi camera capture
After=network.target

[Service]
Type=simple
User=pi
WorkingDirectory=/home/pi/fish_cam_rpi
ExecStart=/home/pi/fish_cam_rpi/bin/fish_cam_rpi \
          --config config/prod.json \
          --timelapse 60
Restart=on-failure
RestartSec=5

[Install]
WantedBy=multi-user.target
```

> `--timelapse 60` captura una imagen cada minuto indefinidamente. Para un
> número limitado añade `--count N`.

Activar el servicio:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now fish_cam
sudo systemctl status fish_cam
```

Ver logs del servicio:

```bash
journalctl -u fish_cam -f
```

## 5. Rotación de logs y limpieza de imágenes

El propio `Logger` escribe con `std::ios::app`; usa `logrotate` para evitar
que el archivo crezca sin límite:

`/etc/logrotate.d/fish_cam`:

```
/srv/fish_cam/logs/camera.log {
  weekly
  rotate 4
  compress
  missingok
  notifempty
  copytruncate
}
```

Limpieza periódica de capturas (cron):

```
# cada día a las 03:00, borra capturas de más de 30 días
0 3 * * * /usr/bin/find /srv/fish_cam/captures -name '*.jpg' -mtime +30 -delete
```

## 6. Envío remoto de imágenes

### rsync

```bash
rsync -avz /srv/fish_cam/captures/ usuario@servidor:/ruta/destino/
```

### ssh con programación

Añade al cron del usuario `pi`:

```
*/30 * * * * scp /srv/fish_cam/captures/*.jpg pi@servidor:/srv/photos/ 2>/dev/null
```

Para ello configura claves SSH (`ssh-keygen` + `ssh-copy-id`).

## 7. Actualización del software

```bash
cd ~/fish_cam_rpi
git pull
make
sudo systemctl restart fish_cam
```

## 8. Salud del sistema

- `scripts/test_camera.sh`: verifica la cámara.
- `vcgencmd get_camera`: estado de la cámara legacy.
- `vcgencmd measure_temp`: temperatura (mantener < 80 °C).
- `journalctl -u fish_cam`: revisa errores de captura.

## 9. Seguridad

- Ejecuta el servicio con un usuario sin privilegios (`User=pi`).
- No uses credenciales en texto plano en cron; usa claves SSH.
- Mantén la Raspberry actualizada: `sudo apt update && sudo apt upgrade`.
- Si la cámara está en red pública, protege el acceso remoto con firewall
  (`sudo ufw allow OpenSSH`).
