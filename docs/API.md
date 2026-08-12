# Documentación de la API

Todos los tipos viven en el namespace `fishcam`. Los encabezados se pueden
incluir individualmente (`#include "camera/CameraManager.hpp"`, etc.) o todos
juntos mediante `#include "fish_cam.hpp"` desde `include/`.

## Logger (`src/utils/Logger.hpp`)

Singleton seguro para hilos con salida a consola y/o archivo.

```cpp
enum class LogLevel { kDebug, kInfo, kWarning, kError, kCritical };
enum class Logger::Output { kConsole, kFile, kBoth };

static Logger& Logger::Instance();
void Logger::Configure(const std::string& log_file, LogLevel min_level, Output output);
void Logger::Log(LogLevel level, const std::string& message);
static void Logger::Debug/Info/Warning/Error/Critical(const std::string&);
static LogLevel Logger::ParseLevel(const std::string& name);
```

Ejemplo:

```cpp
using namespace fishcam;
Logger::Instance().Configure("logs/app.log", LogLevel::kInfo, Logger::Output::kBoth);
Logger::Info("Inicializando cámara...");
```

## TimestampManager (`src/timestamp/TimestampManager.hpp`)

```cpp
static std::string TimestampManager::Now(const std::string& format = "%Y-%m-%d %H:%M:%S");
static std::string TimestampManager::FilenameStamp();        // YYYYMMDD_HHMMSS
static std::string TimestampManager::FormatFilename(const std::string& pattern,
                                                    const std::string& stamp = "");
static std::string TimestampManager::BuildFilename(const std::string& extension,
                                                   const std::string& prefix = "fishcam");
```

Ejemplo:

```cpp
std::string stamp = TimestampManager::Now();                    // 2026-08-11 22:40:01
std::string name  = TimestampManager::BuildFilename("jpg");     // fishcam_20260811_224001.jpg
```

## ImageProcessor (`src/image/ImageProcessor.hpp`)

```cpp
struct OverlayOptions {
  double font_scale = 1.2;
  int thickness = 2;
  cv::Scalar color{0, 255, 255};   // BGR
  int margin = 10;
};

struct EncodingOptions {
  int jpeg_quality = 100;          // 1..100
  int png_compression = 0;         // 0..9
};

class ImageProcessor {
 public:
  void OverlayTimestamp(cv::Mat& image, const std::string& text,
                        const OverlayOptions& options = {}) const;
  bool Save(const cv::Mat& image, const std::string& file_path,
            const EncodingOptions& options = {}) const;
};
```

Ejemplo:

```cpp
ImageProcessor processor;
processor.OverlayTimestamp(frame, TimestampManager::Now());
EncodingOptions encoding;          // calidad máxima por defecto
bool ok = processor.Save(frame, "captures/fishcam_20260811_224001.jpg", encoding);
```

## CameraManager (`src/camera/CameraManager.hpp`)

Envuelve la cámara OV5647 mediante **raspicam**. Sigue RAII: el destructor
libera la cámara. `Initialize()` reintenta la conexión 3 veces; cada captura
tiene un timeout de 5 s.

```cpp
enum class CameraManager::Encoding { kJpeg, kPng };

struct CameraManager::Config {
  int width = 1920, height = 1080, frame_rate = 30;
  int quality = 100;
  int iso = 0;                     // 0 = auto
  int brightness = 50, contrast = 50, saturation = 50, sharpness = 50;
  std::string exposure = "auto";   // auto|off|night|backlight|sports|...
  int rotation = 0;
  bool horizontal_flip = false, vertical_flip = false, show_preview = false;
  int capture_timeout_ms = 5000;
  int retry_attempts = 3;
  Encoding encoding = Encoding::kJpeg;
};

class CameraManager {
 public:
  explicit CameraManager(const Config& config);
  ~CameraManager();
  bool Initialize();
  bool IsReady() const;
  bool Capture(cv::Mat& frame);
  bool CaptureAsync(std::function<void(bool, cv::Mat)> callback);
  int CaptureBurst(int count, std::vector<cv::Mat>& frames);
  void Shutdown();
  Config GetConfig() const;
  std::string GetLastError() const;
  static std::vector<std::string> DetectCameras();
};
```

Ejemplo:

```cpp
CameraManager::Config cfg;
cfg.width = 1920; cfg.height = 1080; cfg.quality = 100;
CameraManager camera(cfg);
if (!camera.Initialize()) {
  Logger::Critical(camera.GetLastError());
  return 1;
}
cv::Mat frame;
if (camera.Capture(frame)) {
  // procesar frame...
}
// El destructor libera la cámara automáticamente.
```

## ConfigManager (`src/config/ConfigManager.hpp`)

Carga, valida y guarda la configuración JSON (nlohmann/json).

```cpp
struct CameraSettings;   // width, height, format, quality, iso, brightness, ...
struct OutputSettings;   // directory, filename_pattern, max_size_mb, overwrite, overlay
struct LoggingSettings;  // level, file, max_size_mb, backup_count
struct AppSettings { CameraSettings camera; OutputSettings output; LoggingSettings logging; };

class ConfigManager {
 public:
  static AppSettings LoadDefaults();
  static bool LoadFromFile(const std::string& path, AppSettings& out, std::string& error);
  static bool Save(const std::string& path, const AppSettings& settings);
};
```

## Convenciones

- Todos los métodos estáticos de utilidad están en `TimestampManager`.
- El `Logger` es un singleton; no lo copies (`Logger(const Logger&) = delete`).
- `CameraManager` es no copiable; la cámara no se comparte entre hilos salvo
  por las operaciones internas protegidas con `std::mutex`.
- Los errores se reportan por valor de retorno (`bool`/`int`) y a través de
  `Logger`; `CameraManager::GetLastError()` devuelve el último mensaje.
