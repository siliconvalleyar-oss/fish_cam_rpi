/**
 * @file ImageProcessor.cpp
 * @brief Implementation of the image post-processing utilities.
 */

#include "image/ImageProcessor.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <sys/stat.h>
#include <vector>

#include "utils/Logger.hpp"

namespace fishcam {

namespace {

/**
 * @brief Recursively creates every directory component of a path.
 */
bool MakeDirectoryRecursive(const std::string& dir) {
  if (dir.empty() || dir == "/") {
    return true;
  }
  std::string current = (dir.front() == '/') ? "/" : "";
  std::string::size_type start = (dir.front() == '/') ? 1 : 0;
  while (start <= dir.size()) {
    const std::string::size_type pos = dir.find('/', start);
    const std::string::size_type end =
        (pos == std::string::npos) ? dir.size() : pos;
    const std::string segment = dir.substr(start, end - start);
    if (!segment.empty()) {
      if (current == "/") {
        current += segment;
      } else if (current.empty()) {
        current = segment;
      } else {
        current += "/" + segment;
      }
      if (::mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
        return false;
      }
    }
    if (pos == std::string::npos) {
      break;
    }
    start = pos + 1;
  }
  return true;
}

std::string ToLower(std::string text) {
  for (char& c : text) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return text;
}

}  // namespace

void ImageProcessor::OverlayTimestamp(cv::Mat& image, const std::string& text,
                                      const OverlayOptions& options) const {
  if (image.empty() || text.empty()) {
    return;
  }

  const int font_face = cv::FONT_HERSHEY_SIMPLEX;
  int baseline = 0;
  const cv::Size text_size = cv::getTextSize(
      text, font_face, options.font_scale, options.thickness, &baseline);

  const cv::Point origin(options.margin, options.margin + text_size.height);
  const cv::Rect background(
      origin.x - 6, origin.y - text_size.height - 6,
      text_size.width + 12, text_size.height + baseline + 12);

  cv::rectangle(image, background, cv::Scalar(0, 0, 0), cv::FILLED, cv::LINE_8);
  cv::putText(image, text, origin, font_face, options.font_scale, options.color,
              options.thickness, cv::LINE_AA);
}

bool ImageProcessor::Save(const cv::Mat& image, const std::string& file_path,
                          const EncodingOptions& options) const {
  if (image.empty()) {
    Logger::Error("Cannot save an empty image");
    return false;
  }

  std::string extension;
  const std::string::size_type dot = file_path.find_last_of('.');
  const std::string::size_type slash = file_path.find_last_of('/');
  if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
    extension = ToLower(file_path.substr(dot));
  }

  std::vector<int> params;
  if (extension == ".jpg" || extension == ".jpeg") {
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(std::clamp(options.jpeg_quality, 1, 100));
  } else if (extension == ".png") {
    params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    params.push_back(std::clamp(options.png_compression, 0, 9));
  }

  const std::string::size_type last_slash = file_path.find_last_of('/');
  if (last_slash != std::string::npos &&
      !MakeDirectoryRecursive(file_path.substr(0, last_slash))) {
    Logger::Error("Could not create output directory for " + file_path);
    return false;
  }

  return cv::imwrite(file_path, image, params);
}

}  // namespace fishcam
