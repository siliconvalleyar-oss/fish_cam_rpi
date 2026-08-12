/**
 * @file ImageProcessor.hpp
 * @brief Image post-processing: timestamp overlay and maximum-quality saving.
 *
 * @copyright MIT License - fish_cam_rpi contributors
 */

#ifndef FISH_CAM_IMAGE_IMAGEPROCESSOR_HPP_
#define FISH_CAM_IMAGE_IMAGEPROCESSOR_HPP_

#include <opencv2/core.hpp>

#include <string>

namespace fishcam {

/** @brief Options that control how the timestamp is drawn. */
struct OverlayOptions {
  double font_scale = 1.2;            ///< Font scale factor.
  int thickness = 2;                  ///< Line thickness.
  cv::Scalar color{0, 255, 255};      ///< Text color in BGR (default: yellow).
  int margin = 10;                    ///< Margin from the top-left corner.
};

/** @brief Options for image encoding. */
struct EncodingOptions {
  int jpeg_quality = 100;             ///< JPEG quality 1..100 (100 = best).
  int png_compression = 0;            ///< PNG compression 0..9 (0 = best quality).
};

/**
 * @brief Responsible for image post-processing (SRP: drawing and encoding).
 *
 * This class does not own the camera; it operates on cv::Mat frames supplied
 * by the caller.
 */
class ImageProcessor {
 public:
  /**
   * @brief Draws a timestamp in the top-left corner of the image.
   * @param image Image modified in-place.
   * @param text Timestamp text to draw.
   * @param options Rendering options.
   */
  void OverlayTimestamp(cv::Mat& image, const std::string& text,
                        const OverlayOptions& options = {}) const;

  /**
   * @brief Saves an image with maximum quality for the detected format.
   * @param image Image to save.
   * @param file_path Destination path. The extension (.jpg/.jpeg/.png)
   *                  selects the encoder.
   * @param options Encoding parameters.
   * @return true on success, false otherwise.
   */
  bool Save(const cv::Mat& image, const std::string& file_path,
            const EncodingOptions& options = {}) const;
};

}  // namespace fishcam

#endif  // FISH_CAM_IMAGE_IMAGEPROCESSOR_HPP_
