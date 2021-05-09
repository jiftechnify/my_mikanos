#include "grayscale_image.hpp"
#include "logger.hpp"

namespace {
  PixelColor grayscale_4grads_color_palette[4] = {
    {0, 0, 0},        // 0: black
    {102, 102, 102},  // 1: dark gray
    {187, 187, 187},  // 2: light gray
    {255, 255, 255},  // 3: white
  };
}

Grayscale4GradsImage::Grayscale4GradsImage(int width, int height, uint8_t* bmp_data) : width_{width}, height_{height}, bmp_data_{bmp_data} {
  Log(kWarn, "w: %d, h: %d\n", width, height);
}

const uint8_t Grayscale4GradsImage::ColorNumberAt(Vector2D<int> pos) const {
  auto pixel = pos.x + pos.y * width_;
  return (bmp_data_[pixel / 4] >> ((3 - pixel % 4) * 2)) & 0b11;
}

void DrawGrayscale4GradsImage(PixelWriter& writer, Vector2D<int> pos, const Grayscale4GradsImage& img) {
  for (int dy = 0; dy < img.Height(); ++dy) {
    for (int dx = 0; dx < img.Width(); ++dx) {
      writer.Write(pos + Vector2D<int>{dx, dy}, grayscale_4grads_color_palette[img.ColorNumberAt({dx, dy})]);
    }
  }
}

void DrawGrayscale4GradsImageScaled(PixelWriter& writer, Vector2D<int> pos, int scale, const Grayscale4GradsImage& img) {
  if (scale <= 0) {
    return;
  }
  auto draw_pixel = [&writer, pos, scale](Vector2D<int> px_pos, const PixelColor& color) {
    for (int dy = 0; dy < scale; ++dy) {
      for (int dx = 0; dx < scale; ++dx) {
        writer.Write(pos + Vector2D<int>{px_pos.x * scale + dx, px_pos.y * scale + dy}, color);
      }
    }
  };

  for (int dy = 0; dy < img.Height(); ++dy) {
    for (int dx = 0; dx < img.Width(); ++dx) {
      draw_pixel({dx, dy}, grayscale_4grads_color_palette[img.ColorNumberAt({dx, dy})]);
    }
  }
}

const Grayscale4GradsImage MakeGrayscaleAegis() {
  return Grayscale4GradsImage{aegis_width, aegis_height, grayscale_aegis_bmp_data};
}
