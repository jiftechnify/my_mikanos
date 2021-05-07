#include "grayscale_image.hpp"

namespace {
  PixelColor grayscale_4grads_color_palette[4] = {
    {0, 0, 0},        // 0: black
    {102, 102, 102},  // 1: dark gray
    {187, 187, 187},  // 2: ligth gray
    {255, 255, 255},  // 3: white
  };
}

void WriteGrayscale4GradsImage(PixelWriter& writer, Vector2D<int> pos, const Grayscale4GradsImage& img) {
  for (int dy = 0; dy < img.height; ++dy) {
    for (int dx = 0; dx < img.width; ++dx) {
      auto px = dx + dy * img.width;
      auto color_num = (img.bmp_data[px / 4] >> ((3 - px % 4) * 2)) & 0b11;
      writer.Write(pos + Vector2D<int>{dx, dy}, grayscale_4grads_color_palette[color_num]);
    }
  }
}

