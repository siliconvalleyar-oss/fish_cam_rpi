# Guía para contribuir

¡Gracias por querer contribuir a **fish_cam_rpi**! Estas directrices mantienen
el código coherente y fácil de mantener.

## Flujo de trabajo

1. Crea una rama desde `main`:

   ```bash
   git checkout -b feature/mi-cambio
   ```

2. Realiza tus cambios con commits pequeños y descriptivos.
3. Asegúrate de que todo compila y las pruebas pasan:

   ```bash
   make -j4
   make test
   ./tests/integration_tests/test_integration.sh
   ```

4. Abre una *pull request* describiendo el cambio y su motivación.

## Convenciones de código

- **C++17** (o superior).
- Estilo **Google C++ Style Guide**:
  - indentación de 2 espacios, sin tabs;
  - nombres de clases `PascalCase`, funciones y variables `camelCase`,
    constantes `kCamelCase`, miembros `snake_case_`;
  - llaves estilo Allman o adjuntas (elige una y sé consistente; el proyecto
    usa llaves adjuntas).
- **Comentarios Doxygen** en todos los encabezados públicos
  (`@file`, `@brief`, descripción de parámetros con `@param`, retorno con
  `@return`).
- **RAII**: los recursos deben liberarse en destructores.
- **SRP**: cada clase debe tener una única responsabilidad; evita clases
  "todopoderosas".
- Sin errores de compilación con `-Wall -Wextra`.

## Estructura de un módulo

```
src/<modulo>/
├── <Clase>.hpp   → declaración + documentación Doxygen
└── <Clase>.cpp   → implementación
```

Las cabeceras se agrupan en `include/fish_cam.hpp`.

## Pruebas

- Las pruebas unitarias viven en `tests/unit_tests/` y usan `assert`.
- Las pruebas de integración viven en `tests/integration_tests/` (shell) y
  requieren cámara.
- Añade pruebas para cualquier funcionalidad nueva; `make test` debe pasar.

## Documentación

- Actualiza `docs/API.md` si cambias la API pública.
- Registra cambios notables en `docs/CHANGELOG.md`.
- Añade ejemplos en `docs/USAGE.md` si introduces nueva funcionalidad.

## Informe de errores

Incluye en el *issue*:

- Modelo de Raspberry Pi y versión del sistema (`cat /etc/os-release`).
- Salida de `./scripts/test_camera.sh`.
- El comando exacto ejecutado y el log completo (`logs/camera.log`).

## Licencia

Al contribuir aceptas que tu código se distribuya bajo la licencia MIT del
proyecto (ver `docs/LICENSE.md`).
