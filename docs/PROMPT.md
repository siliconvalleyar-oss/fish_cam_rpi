================================================================================
PROMPT COMPLETO PARA IA - PROYECTO fish_cam_rpi (VERSIÓN FINAL MERGE)
================================================================================

[INSTRUCCIONES GENERALES]
Actúa como un asistente experto en desarrollo de software para sistemas embebidos,
especializado en Raspberry Pi, C++, y sistemas de captura de imágenes.

[OBJETIVO PRINCIPAL]
Generar la estructura completa de un proyecto de software para Raspberry Pi
que permita capturar imágenes desde una cámara OV5647 (ángulo 130 grados)
usando C++, con organización profesional y documentación completa.

[REQUERIMIENTOS DEL PROYECTO]

1. NOMBRE DEL PROYECTO: fish_cam_rpi

2. ESTRUCTURA DE DIRECTORIOS:
   - bin/         : Archivos binarios ejecutables
   - obj/         : Objetos compilados (manteniendo estructura de src/)
   - src/         : Código fuente (*.cpp)
   - include/     : Archivos de cabecera (*.hpp, *.h)
   - docs/        : Documentación en formato Markdown (*.md)
   - config/      : Archivos de configuración
   - scripts/     : Scripts de instalación y prueba
   - tests/       : Pruebas unitarias y de integración
   - logs/        : Directorio para logs

3. ESTRUCTURA DE ARCHIVOS COMPLETA:
fish_cam_rpi/
├── bin/
│   └── (ejecutables generados)
├── obj/
│   └── (objetos compilados)
├── src/
│   ├── main.cpp
│   ├── camera/
│   │   ├── CameraManager.cpp
│   │   └── CameraManager.hpp
│   ├── image/
│   │   ├── ImageProcessor.cpp
│   │   └── ImageProcessor.hpp
│   ├── timestamp/
│   │   ├── TimestampManager.cpp
│   │   └── TimestampManager.hpp
│   ├── config/
│   │   ├── ConfigManager.cpp
│   │   └── ConfigManager.hpp
│   └── utils/
│       ├── Logger.cpp
│       └── Logger.hpp
├── include/
│   └── (archivos de cabecera públicos)
├── docs/
│   ├── README.md
│   ├── INSTALL.md
│   ├── USAGE.md
│   ├── API.md
│   ├── CHANGELOG.md
│   ├── CONTRIBUTING.md
│   ├── LICENSE.md
│   ├── ARCHITECTURE.md
│   └── DEPLOYMENT.md
├── config/
│   └── camera_config.json
├── scripts/
│   ├── setup.sh
│   ├── capture_demo.sh
│   └── test_camera.sh
├── tests/
│   ├── unit_tests/
│   └── integration_tests/
├── Makefile
├── .gitignore
└── .git/config

4. COMPONENTES REQUERIDOS:
   a) Sistema de captura de imágenes con cámara OV5647
   b) Impresión de timestamp (fecha y hora) en la imagen capturada
   c) Formato de salida: JPG o PNG con máxima calidad (100%)
   d) Captura bajo demanda (cuando se solicite)
   e) Sistema de compilación con Makefile
   f) Control de versiones con Git (nombre: fish_cam_rpi)

5. DOCUMENTACIÓN REQUERIDA (en docs/):
   - README.md           : Descripción general del proyecto
   - INSTALL.md          : Instrucciones de instalación y dependencias
   - USAGE.md            : Guía de uso y ejemplos
   - API.md              : Documentación técnica de la API
   - CHANGELOG.md        : Registro de cambios
   - CONTRIBUTING.md     : Guía para contribuir
   - LICENSE.md          : Información de licencia
   - ARCHITECTURE.md     : Arquitectura del sistema
   - DEPLOYMENT.md       : Guía de despliegue

6. DEPENDENCIAS:
   - libraspicam-dev (para cámara)
   - libopencv-dev (para procesamiento de imágenes)
   - libjpeg-dev (para codificación JPEG)
   - libpng-dev (para codificación PNG)
   - build-essential (compiladores)
   - git (control de versiones)
   - nlohmann-json-dev (para configuración JSON)

   Comandos de instalación:
   sudo apt-get update
   sudo apt-get install -y libraspicam-dev libopencv-dev libjpeg-dev libpng-dev build-essential git nlohmann-json3-dev

7. ESPECIFICACIONES TÉCNICAS:
   - Lenguaje: C++ (estándar C++17 o superior)
   - Biblioteca: raspicam o MMAL/V4L2 para cámara
   - Procesamiento de imágenes: OpenCV o libjpeg
   - Sistema: Raspberry Pi OS (Debian-based)
   - Cámara: OV5647 con lente de 130 grados

8. CONFIGURACIÓN DE CÁMARA OV5647 130°:
   - Resolución: 1920x1080 (por defecto), soporte para 2592x1944
   - ISO: automático (100-800)
   - Velocidad de obturación: automática
   - Balance de blancos: automático
   - Modo de enfoque: fijo (distancia 0.5m - infinito)
   - Rotación: 0° (configurable)
   - Calidad JPEG: 100% (máxima)
   - Formato de salida: JPG (por defecto) con opción PNG

9. FUNCIONALIDADES ESPECÍFICAS:
   - Inicialización de la cámara con parámetros optimizados
   - Configuración de resolución y calidad desde archivo JSON
   - Captura síncrona con timeout de 5 segundos
   - Overlay de timestamp en formato "YYYY-MM-DD HH:MM:SS"
   - Almacenamiento en archivo: "fishcam_YYYYMMDD_HHMMSS.jpg/png"
   - Manejo de errores robusto con reintentos (3 intentos)
   - Logging detallado con niveles (DEBUG, INFO, WARNING, ERROR, CRITICAL)
   - Captura en ráfaga (N imágenes consecutivas)
   - Modo timelapse (captura cada X segundos/minutos)

10. MANEJO DE ERRORES:
    - Cámara no detectada → 3 intentos de reconexión, mostrar mensaje de error
    - Permisos insuficientes → sugerir comando de solución
    - Espacio en disco insuficiente → verificar y advertir si < 100MB
    - Formato no soportado → fallback a JPG
    - Timeout de captura (5 segundos máximo)

    Mensajes de error específicos:
    - "ERROR: No se detectó cámara OV5647. Verifique conexión."
    - "ERROR: Permisos insuficientes para acceder a /dev/video0"
    - "ERROR: Espacio en disco insuficiente (< 100MB disponibles)"
    - "ERROR: Timeout en captura (5s)"

11. OPCIONES DE LÍNEA DE COMANDOS:
    --capture            : Captura simple
    --output <archivo>   : Especificar nombre de archivo
    --format <jpg|png>   : Formato de salida
    --burst <N>          : Ráfaga de N imágenes
    --timelapse <S>      : Timelapse cada S segundos
    --config <archivo>   : Usar archivo de configuración
    --help               : Mostrar ayuda

12. SISTEMA DE CONFIGURACIÓN (camera_config.json):
{
  "camera": {
    "resolution": {"width": 1920, "height": 1080},
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
    "overwrite": false
  },
  "logging": {
    "level": "info",
    "file": "./logs/camera.log",
    "max_size_mb": 5,
    "backup_count": 3
  }
}

13. SISTEMA DE LOGGING:
    Niveles: DEBUG, INFO, WARNING, ERROR, CRITICAL

    Ejemplos de mensajes:
    [INFO] Inicializando cámara OV5647...
    [INFO] Cámara detectada en /dev/video0
    [INFO] Configuración aplicada: 1920x1080, JPG 100%
    [INFO] Captura exitosa: fishcam_20241115_143022.jpg (2.3MB)
    [INFO] Timestamp añadido correctamente
    [WARNING] Espacio en disco bajo (< 100MB)
    [WARNING] Batería baja (< 15%)
    [ERROR] Timeout en captura (5s)
    [DEBUG] Buffer de imagen: 0x7f8a4c001000, tamaño: 2,457,600 bytes
    [CRITICAL] Error crítico: No se puede inicializar la cámara

14. PRUEBAS SUGERIDAS:
    a) Prueba de detección de cámara
       → ./test_camera.sh --detect

    b) Prueba de captura simple
       → ./test_camera.sh --capture-test

    c) Prueba de estrés (100 capturas consecutivas)
       → ./test_camera.sh --stress --count 100

    d) Prueba de diferentes resoluciones
       → ./test_camera.sh --test-resolutions

    e) Prueba de formatos (JPG, PNG)
       → ./test_camera.sh --test-formats

    f) Prueba de timestamp (verificar overlay)
       → ./test_camera.sh --test-timestamp

15. REQUISITOS DE CALIDAD DE CÓDIGO:
    - Estilo de código Google C++ Style Guide
    - Comentarios Doxygen para documentación automática
    - Manejo de excepciones
    - RAII (Resource Acquisition Is Initialization)
    - Separación de responsabilidades (SRP)
    - Interfaces claras y bien definidas
    - C++17 o superior

16. EJEMPLOS DE API:
    class CameraManager {
    public:
        bool initialize(int width, int height, int quality);
        bool captureImage(const std::string& filename);
        bool setFormat(ImageFormat format);
        bool setResolution(int width, int height);
        void setTimestampOverlay(bool enabled);
        std::string getLastError();
    };

    class ImageProcessor {
    public:
        bool addTimestamp(unsigned char* imageData, size_t size, const std::string& timestamp);
        bool saveImage(const unsigned char* imageData, size_t size, const std::string& filename, ImageFormat format);
        bool resizeImage(unsigned char* imageData, int newWidth, int newHeight);
    };

    class TimestampManager {
    public:
        std::string getCurrentTimestamp();
        std::string generateFilename(const std::string& pattern, const std::string& extension);
    };

    class Logger {
    public:
        void log(LogLevel level, const std::string& message);
        void setLogLevel(LogLevel level);
        void setLogFile(const std::string& filename);
    };

    class ConfigManager {
    public:
        bool loadConfig(const std::string& filename);
        bool saveConfig(const std::string& filename);
        std::string getValue(const std::string& key);
        void setValue(const std::string& key, const std::string& value);
    };

17. MENSAJES DE ESTADO:
    - "Inicializando cámara OV5647..."
    - "Cámara detectada correctamente"
    - "Capturando imagen..."
    - "Imagen capturada: [tamaño] MB, [resolución]"
    - "Añadiendo timestamp..."
    - "Guardando imagen: [nombre_archivo]"
    - "Captura completada en [tiempo] ms"
    - "Timestamp añadido correctamente"

18. OPTIMIZACIONES:
    - Usar memoria compartida para transferencia de datos
    - Buffer de imágenes (2-3 frames en cola)
    - Compresión en tiempo real
    - Hilos separados para captura y procesamiento
    - Prioridad alta para el proceso de captura (nice -n -10)
    - Desactivar preview de cámara para ahorrar recursos

19. FUNCIONALIDADES OPCIONALES ADICIONALES:
    - Modo ráfaga: capturar N imágenes consecutivas
    - Timelapse: capturar cada X segundos/minutos
    - Detección de movimiento básica (diferencia de frames)
    - Envío por red (FTP/HTTP) después de capturar (opcional)
    - Compresión automática si el archivo es muy grande
    - Rotación automática según orientación
    - Filtros básicos (escala de grises, sepia, etc.)
    - Verificación de integridad de la imagen capturada

20. EJEMPLOS DE USO EN LÍNEA DE COMANDOS:
    ./bin/fish_cam_rpi --capture                    # Captura simple
    ./bin/fish_cam_rpi --capture --output test.jpg  # Especificar nombre
    ./bin/fish_cam_rpi --capture --format png       # Formato PNG
    ./bin/fish_cam_rpi --burst --count 10           # Ráfaga de 10 fotos
    ./bin/fish_cam_rpi --timelapse --interval 30    # Cada 30 segundos
    ./bin/fish_cam_rpi --config config.json         # Usar configuración
    ./bin/fish_cam_rpi --help                       # Mostrar ayuda

21. CONFIGURACIÓN DE GIT:
    - Repositorio: fish_cam_rpi
    - Credenciales: usar las de la PC local
    - Archivo .git/config con configuración local
    - .gitignore con patrones para:
      - bin/ y obj/ directorios
      - Archivos compilados (*.o, *.a, *.so)
      - Archivos de log (*.log)
      - Archivos de captura (*.jpg, *.png)
      - Directorio .vscode/ o .idea/
      - Archivos temporales (*~, *.swp)

22. SCRIPT DE SETUP (setup.sh):
    #!/bin/bash
    echo "Instalando dependencias..."
    sudo apt-get update
    sudo apt-get install -y libraspicam-dev libopencv-dev libjpeg-dev libpng-dev build-essential git nlohmann-json3-dev
    echo "Configurando permisos de cámara..."
    sudo usermod -a -G video $USER
    echo "Compilando proyecto..."
    make clean
    make
    echo "Instalación completada"

23. SCRIPT DE DEMOSTRACIÓN (capture_demo.sh):
    #!/bin/bash
    echo "=== fish_cam_rpi Demo ==="
    echo "Capturando imagen simple..."
    ./bin/fish_cam_rpi --capture
    echo "Capturando ráfaga de 5 imágenes..."
    ./bin/fish_cam_rpi --burst 5
    echo "Demo completada"

24. SCRIPT DE PRUEBA (test_camera.sh):
    #!/bin/bash
    echo "=== Probando cámara OV5647 ==="
    echo "1. Detectando cámara..."
    vcgencmd get_camera
    echo "2. Probando captura..."
    ./bin/fish_cam_rpi --capture --output test.jpg
    echo "3. Verificando imagen..."
    if [ -f "test.jpg" ]; then
        echo "✓ Imagen creada correctamente"
        ls -lh test.jpg
    else
        echo "✗ Error en captura"
    fi

25. CONSIDERACIONES FINALES:
    - El código debe ser compilable en Raspberry Pi 3/4/5
    - La cámara debe ser detectada automáticamente
    - El timestamp debe estar en formato "YYYY-MM-DD HH:MM:SS"
    - La imagen debe guardarse con nombre: "fishcam_YYYYMMDD_HHMMSS.jpg/png"
    - Incluir todos los scripts de prueba y demostración
    - Documentar todas las dependencias necesarias
    - El proyecto debe ser autocontenido, compilable y funcional

[FORMATO DE SALIDA DEL PROMPT]

Genera el proyecto completo con TODOS los archivos necesarios. Para cada archivo,
especifica su ruta completa y su contenido completo. El código debe ser
compilable y funcional en Raspberry Pi 3/4/5 con Raspberry Pi OS.

Asegúrate de incluir:
1. Todos los archivos .cpp y .hpp con su implementación completa
2. Makefile completo con todas las reglas necesarias
3. .gitignore con los patrones adecuados
4. Todos los archivos .md de documentación
5. Archivos de configuración JSON
6. Scripts de shell para instalación y pruebas
7. Configuración de Git con credenciales locales
8. Manejo de errores completo
9. Sistema de logging funcional
10. Opciones de línea de comandos implementadas

[INSTRUCCIÓN FINAL PARA LA IA]
Genera el proyecto completo con todos los archivos necesarios,
asegurando que cada archivo tenga el contenido apropiado y que
el sistema compile correctamente. El prompt debe ser autocontenido
y claro para que la IA pueda generar el proyecto sin ambigüedades.
El proyecto debe ser funcional inmediatamente después de clonar y
ejecutar el script setup.sh y make.

================================================================================
