#include <cstdint>
#include <cstddef>
#include "frame_buffer_config.hpp"

void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
}

struct PixelColor {
  uint8_t r, g, b;
};

class PixelWriter {
  public:
    PixelWriter(const FrameBufferConfig& config): config_{config} {
    }
    virtual ~PixelWriter() = default;
    virtual void Write(int x, int y, const PixelColor& c) = 0;

  protected:
    uint8_t* PixelAt(int x, int y) {
      return config_.frame_buffer + 4 * (config_.pixels_per_scan_line * y + x);
    }

  private:
    const FrameBufferConfig& config_;
};

class RGBResv8BitPerColorPixelWriter : public PixelWriter {
  public:
    using PixelWriter::PixelWriter;

    virtual void Write(int x, int y, const PixelColor& c) override {
      auto p = PixelAt(x, y);
      p[0] = c.r;
      p[1] = c.g;
      p[2] = c.b;
    }
};

class BGRResv8BitPerColorPixelWriter : public PixelWriter {
  public:
    using PixelWriter::PixelWriter;

    virtual void Write(int x, int y, const PixelColor& c) override {
      auto p = PixelAt(x, y);
      p[0] = c.b;
      p[1] = c.g;
      p[2] = c.r;
    }
};

const PixelColor BK = {0, 0, 0};
const PixelColor DG = {102, 102, 102};
const PixelColor LG = {187, 187, 187};
const PixelColor WH = {255, 255, 255};
const int AEGIS_WIDTH = 20;
const int AEGIS_HEIGHT = 34;
const PixelColor AEGIS_BMP[AEGIS_WIDTH * AEGIS_HEIGHT] = {
  WH, WH, WH, WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, WH, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, BK, BK, DG, DG, DG, DG, DG, DG, BK, BK, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, BK, DG, DG, LG, LG, LG, LG, LG, LG, DG, DG, BK, WH, WH, WH, WH,
  WH, WH, WH, BK, DG, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, DG, BK, WH, WH, WH,
  WH, WH, BK, DG, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, DG, BK, WH, WH,
  WH, WH, BK, DG, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, DG, BK, WH, WH,
  WH, BK, DG, LG, LG, WH, WH, LG, LG, LG, LG, LG, LG, LG, LG, LG, LG, DG, BK, WH,
  WH, BK, DG, LG, LG, BK, BK, BK, WH, LG, LG, WH, BK, BK, BK, LG, LG, DG, BK, WH,
  BK, DG, DG, LG, BK, WH, BK, BK, WH, WH, LG, WH, BK, BK, WH, BK, LG, DG, DG, BK,
  BK, DG, DG, LG, WH, WH, WH, BK, WH, WH, WH, WH, WH, BK, WH, WH, LG, DG, DG, BK,
  BK, DG, DG, LG, WH, WH, BK, BK, WH, WH, WH, WH, BK, BK, WH, WH, LG, DG, DG, BK,
  WH, BK, DG, LG, WH, WH, WH, WH, WH, BK, WH, WH, WH, WH, WH, WH, LG, DG, BK, WH,
  WH, WH, BK, LG, BK, WH, WH, WH, WH, WH, WH, WH, WH, WH, WH, BK, LG, BK, WH, WH,
  WH, WH, WH, BK, LG, BK, WH, WH, WH, BK, BK, WH, WH, WH, BK, LG, BK, WH, WH, WH,
  WH, WH, WH, WH, BK, BK, BK, BK, BK, WH, WH, BK, BK, BK, BK, BK, WH, WH, WH, WH,
  WH, WH, WH, WH, BK, WH, BK, BK, BK, BK, BK, BK, BK, BK, BK, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, BK, BK, WH, WH, BK, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, BK, WH, WH, BK, BK, WH, WH,
  WH, WH, WH, WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, WH, BK, WH, WH, WH, BK, WH,
  WH, WH, WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, BK, BK, WH, BK, WH, WH, BK, WH,
  WH, WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, WH, BK, BK, WH, WH,
  WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, WH, WH, WH, WH,
  WH, WH, WH, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, WH, WH, WH,
  WH, WH, BK, WH, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, BK, WH, BK, WH, WH,
  WH, WH, WH, BK, WH, WH, BK, BK, BK, BK, BK, BK, BK, BK, WH, WH, BK, WH, WH, WH,
  WH, WH, WH, WH, BK, BK, WH, WH, WH, WH, WH, WH, WH, WH, BK, BK, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, WH, BK, BK, BK, BK, BK, BK, BK, BK, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, WH, BK, WH, WH, BK, DG, WH, WH, BK, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, WH, BK, DG, DG, BK, DG, DG, WH, BK, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, WH, BK, DG, DG, BK, BK, DG, DG, BK, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, WH, BK, DG, DG, BK, WH, BK, BK, WH, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, BK, DG, DG, DG, BK, WH, WH, WH, WH, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, BK, WH, WH, WH, BK, WH, WH, WH, WH, WH, WH, WH, WH, WH, WH,
  WH, WH, WH, WH, WH, WH, BK, BK, BK, WH, WH, WH, WH, WH, WH, WH, WH, WH, WH, WH,
};

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

void WriteScaledPixel(PixelWriter* writer, int scale, int x, int y, const PixelColor& c) {
  for (int px = 0; px < scale; ++px) {
    for (int py = 0; py < scale; ++py) {
      writer->Write(x * scale + px, y * scale + py, c);
    }
  }
}

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config) {
  switch (frame_buffer_config.pixel_format) {
    case kPixelRGBResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        RGBResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
    case kPixelBGRResv8BitPerColor:
      pixel_writer = new(pixel_writer_buf)
        BGRResv8BitPerColorPixelWriter{frame_buffer_config};
      break;
  }

  for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x) {
    for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y) {
      pixel_writer->Write(x, y, {0, 0, 0});
    }
  }
  for (int x = 0; x < AEGIS_WIDTH; ++x) {
    for (int y = 0; y < AEGIS_HEIGHT; ++y) {
      WriteScaledPixel(pixel_writer, 4, x, y, AEGIS_BMP[x + y * AEGIS_WIDTH]);
    }
  }
  while (1) __asm__("hlt");
}

