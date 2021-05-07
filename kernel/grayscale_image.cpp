#include "grayscale_image.hpp"

namespace {
  PixelColor grayscale_4grads_color_palette[4] = {
    {0, 0, 0},        // 0: black
    {102, 102, 102},  // 1: dark gray
    {187, 187, 187},  // 2: light gray
    {255, 255, 255},  // 3: white
  };

  uint8_t ColorNumberAt(Vector2D<int> pos, const Grayscale4GradsImage& img) {
    auto pixel = pos.x + pos.y * img.width;
    return (img.bmp_data[pixel / 4] >> ((3 - pixel % 4) * 2)) & 0b11;
  }
}

void DrawGrayscale4GradsImage(PixelWriter& writer, Vector2D<int> pos, const Grayscale4GradsImage& img) {
  for (int dy = 0; dy < img.height; ++dy) {
    for (int dx = 0; dx < img.width; ++dx) {
      auto color_num = ColorNumberAt({dx, dy}, img);
      writer.Write(pos + Vector2D<int>{dx, dy}, grayscale_4grads_color_palette[color_num]);
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

  for (int dy = 0; dy < img.height; ++dy) {
    for (int dx = 0; dx < img.width; ++dx) {
      auto color_num = ColorNumberAt({dx, dy}, img);
      draw_pixel({dx, dy}, grayscale_4grads_color_palette[color_num]);
    }
  }
}

