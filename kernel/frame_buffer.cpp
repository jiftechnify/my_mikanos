#include "frame_buffer.hpp"
#include "error.hpp"

namespace {
  int BytesPerPixel(PixelFormat format) {
    switch (format) {
      case kPixelRGBResv8BitPerColor:
      case kPixelBGRResv8BitPerColor:
        return 4;
    }
    return -1;
  }

  uint8_t* FrameAddrAt(Vector2D<int> pos, const FrameBufferConfig& config) {
    return config.frame_buffer + BytesPerPixel(config.pixel_format) * (config.pixels_per_scan_line * pos.y + pos.x);
  }

  int BytesPerScanLine(const FrameBufferConfig& config) {
    return BytesPerPixel(config.pixel_format) * config.pixels_per_scan_line;
  }

  Vector2D<int> FrameBufferSize(const FrameBufferConfig& config) {
    return {static_cast<int>(config.horizontal_resolution), static_cast<int>(config.vertical_resolution)};
  }
}

// フレームバッファの初期化
// 疑似フレームバッファを用いる場合、FrameBufferConfig.buffer_に必要な領域を確保
Error FrameBuffer::Initialize(const FrameBufferConfig& config) {
  config_ = config;

  const auto bytes_per_pixel = BytesPerPixel(config_.pixel_format);
  if (bytes_per_pixel <= 0) {
    return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  if (config_.frame_buffer) {
    // UEFIで取得したフレームバッファを使用する場合
    buffer_.resize(0);
  } else {
    buffer_.resize(bytes_per_pixel * config_.horizontal_resolution * config_.vertical_resolution);
    config_.frame_buffer = buffer_.data();
    config_.pixels_per_scan_line = config.horizontal_resolution;
  }

  switch (config_.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      writer_ = std::make_unique<RGBResv8BitPerColorPixelWriter>(config_);
      break;
    case kPixelBGRResv8BitPerColor:
      writer_ = std::make_unique<BGRResv8BitPerColorPixelWriter>(config_);
      break;
    default:
      return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  return MAKE_ERROR(Error::kSuccess);
}

// このフレームバッファの指定位置にsrcのバッファデータを高速にコピー
Error FrameBuffer::Copy(Vector2D<int> dst_pos, const FrameBuffer& src, const Rectangle<int>& src_area) {
  if (config_.pixel_format != src.config_.pixel_format) {
    return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  const auto bytes_per_pixel = BytesPerPixel(config_.pixel_format);
  if (bytes_per_pixel <= 0) {
    return MAKE_ERROR(Error::kUnknownPixelFormat);
  }

  const Rectangle<int> src_area_shifted{dst_pos, src_area.size};        // コピー元領域(dst座標)
  const Rectangle<int> src_outline{dst_pos - src_area.pos, FrameBufferSize(src.config_)}; // コピー元バッファ全域(dst座標)
  const Rectangle<int> dst_outline{{0, 0}, FrameBufferSize(config_)};   // コピー先バッファ全域(dst座標)

  const auto copy_area = dst_outline & src_outline & src_area_shifted;  // 実際にコピーする領域(dst座標) バッファからはみ出る部分を落とす
  const auto src_start_pos = copy_area.pos - (dst_pos - src_area.pos);  // copy_area始点(src座標)

  // dst/srcそれぞれにおける、コピー始点ピクセルのアドレスを計算
  uint8_t* dst_buf = FrameAddrAt(copy_area.pos, config_);
  const uint8_t* src_buf = FrameAddrAt(src_start_pos, src.config_);

  const auto bytes_per_copy_line = bytes_per_pixel * copy_area.size.x;
  for (int dy = 0; dy < copy_area.size.y; ++dy) {
    memcpy(dst_buf, src_buf, bytes_per_copy_line);
    dst_buf += BytesPerScanLine(config_);
    src_buf += BytesPerScanLine(src.config_);
  }

  return MAKE_ERROR(Error::kSuccess);
}

void FrameBuffer::Move(Vector2D<int> dst_pos, const Rectangle<int>& src) {
  const auto bytes_per_pixel = BytesPerPixel(config_.pixel_format);
  const auto bytes_per_scan_line = BytesPerScanLine(config_);

  if (dst_pos.y < src.pos.y) {  // move up
    uint8_t* dst_buf = FrameAddrAt(dst_pos, config_);
    const uint8_t* src_buf = FrameAddrAt(src.pos, config_);
    for (int y = 0; y < src.size.y; ++y) {
      memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
      dst_buf += bytes_per_scan_line;
      src_buf += bytes_per_scan_line;
    }
  } else { // move down
    uint8_t* dst_buf = FrameAddrAt(dst_pos + Vector2D<int>{0, src.size.y - 1}, config_);
    const uint8_t* src_buf = FrameAddrAt(src.pos + Vector2D<int>{0, src.size.y - 1}, config_);
    for (int y = 0; y < src.size.y; ++y) {
      memcpy(dst_buf, src_buf, bytes_per_pixel * src.size.x);
      dst_buf -= bytes_per_scan_line;
      src_buf -= bytes_per_scan_line;
    }
  }
}

