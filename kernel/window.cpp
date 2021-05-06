#include "window.hpp"
#include "logger.hpp"

Window::Window(int width, int height, PixelFormat shadow_format) : width_{width}, height_{height} {
  // 各ピクセルの色を格納する2次元配列を生成し、指定ウィンドウサイズに合わせて要素数を拡大
  data_.resize(height);
  for (int y = 0; y < height; ++y) {
    data_[y].resize(width);
  }

  FrameBufferConfig config{};
  config.frame_buffer = nullptr;
  config.horizontal_resolution = width;
  config.vertical_resolution = height;
  config.pixel_format = shadow_format;

  if (auto err = shadow_buffer_.Initialize(config)) {
    Log(kError, "failed to initialize shadow buffer: %s at %s:%d\n",
        err.Name(), err.File(), err.Line());
  }
}

void Window::DrawTo(FrameBuffer& dst, Vector2D<int> position) {
  // 透過色が設定されていない場合
  if (!transparent_color_) {
    dst.Copy(position, shadow_buffer_);
    return;
  }

  // 透過色が設定されている場合、透過色の部分は描画しない(透過)
  const auto tc = transparent_color_.value();
  auto& writer = dst.Writer();
  for (int y = 0; y < Height(); ++y) {
    for (int x = 0; x < Width(); ++x) {
      const auto c = At(Vector2D<int>{x, y});
      if (c != tc) {
        writer.Write(position + Vector2D<int>{x, y}, c);
      }
    }
  }
}

void Window::SetTransparentColor(std::optional<PixelColor> c) {
  transparent_color_ = c;
}

Window::WindowWriter* Window::Writer() {
  return &writer_;
}

const PixelColor& Window::At(Vector2D<int> pos) const {
  return data_[pos.y][pos.x];
}

void Window::Write(Vector2D<int> pos, PixelColor c) {
  data_[pos.y][pos.x] = c;
  shadow_buffer_.Writer().Write(pos, c);
}

int Window::Width() const {
  return width_;
}

int Window::Height() const {
  return height_;
}
  
