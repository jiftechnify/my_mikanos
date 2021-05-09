#include "grayscale_image.hpp"
#include "logger.hpp"


Grayscale4GradsImage::Grayscale4GradsImage(int width, int height, uint8_t* bmp_data) 
  : width_{width}, height_{height}, bmp_data_{bmp_data}, color_palette_{grayscale_4grads_default_color_palette} {
}

void Grayscale4GradsImage::SetColorPalette(PixelColor palette[4]) {
  color_palette_ = palette;
}

const uint8_t Grayscale4GradsImage::ColorNumberAt(Vector2D<int> pos) const {
  auto pixel = pos.x + pos.y * width_;
  return (bmp_data_[pixel / 4] >> ((3 - pixel % 4) * 2)) & 0b11;
}

const PixelColor& Grayscale4GradsImage::ColorAt(Vector2D<int> pos) const {
  return color_palette_[ColorNumberAt(pos)];
}

void DrawGrayscale4GradsImage(PixelWriter& writer, Vector2D<int> pos, const Grayscale4GradsImage& img) {
  for (int dy = 0; dy < img.Height(); ++dy) {
    for (int dx = 0; dx < img.Width(); ++dx) {
      writer.Write(pos + Vector2D<int>{dx, dy}, img.ColorAt({dx, dy}));
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
      draw_pixel({dx, dy}, img.ColorAt({dx, dy}));
    }
  }
}

const Grayscale4GradsImage MakeGrayscaleAegis() {
  return Grayscale4GradsImage{aegis_width, aegis_height, grayscale_aegis_bmp_data};
}
