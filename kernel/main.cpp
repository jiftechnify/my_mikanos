#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"

void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
}

const PixelColor BK = {0, 0, 0};
const PixelColor DG = {102, 102, 102};
const PixelColor LG = {187, 187, 187};
const PixelColor WH = {255, 255, 255};
const int AEGIS_WIDTH = 20;
const int AEGIS_HEIGHT = 34;
const int AEGIS_BMP_DATA_SIZE = (AEGIS_WIDTH * AEGIS_HEIGHT + 3) / 4;
const uint8_t AEGIS_BMP_DATA[AEGIS_BMP_DATA_SIZE] = {
  0xff, 0xfc, 0x00, 0x3f, 0xff,
  0xff, 0xc1, 0x55, 0x43, 0xff,
  0xff, 0x16, 0xaa, 0x94, 0xff,
  0xfc, 0x6a, 0xaa, 0xa9, 0x3f,
  0xf1, 0xaa, 0xaa, 0xaa, 0x4f,
  0xf1, 0xaa, 0xaa, 0xaa, 0x4f,
  0xc6, 0xbe, 0xaa, 0xaa, 0x93,
  0xc6, 0x80, 0xeb, 0x02, 0x93,
  0x16, 0x30, 0xfb, 0x0c, 0x94,
  0x16, 0xfc, 0xff, 0xcf, 0x94,
  0x16, 0xf0, 0xff, 0x0f, 0x94,
  0xc6, 0xff, 0xcf, 0xff, 0x93,
  0xf2, 0x3f, 0xff, 0xfc, 0x8f,
  0xfc, 0x8f, 0xc3, 0xf2, 0x3f,
  0xff, 0x00, 0x3c, 0x00, 0xff,
  0xff, 0x30, 0x00, 0x03, 0xff,
  0xff, 0xc0, 0x00, 0x3c, 0xff,
  0xff, 0xfc, 0x00, 0x0f, 0x0f,
  0xff, 0xfc, 0x00, 0x33, 0xf3,
  0xff, 0xf0, 0x00, 0x0c, 0xf3,
  0xff, 0xc0, 0x00, 0x03, 0x0f,
  0xff, 0x00, 0x00, 0x00, 0xff,
  0xfc, 0x00, 0x00, 0x00, 0x3f,
  0xf3, 0x00, 0x00, 0x00, 0xcf,
  0xfc, 0xf0, 0x00, 0x0f, 0x3f,
  0xff, 0x0f, 0xff, 0xf0, 0xff,
  0xff, 0xf0, 0x00, 0x0f, 0xff,
  0xff, 0xf3, 0xc7, 0xcf, 0xff,
  0xff, 0xf1, 0x45, 0xcf, 0xff,
  0xff, 0xf1, 0x41, 0x4f, 0xff,
  0xff, 0xf1, 0x4c, 0x3f, 0xff,
  0xff, 0xc5, 0x4f, 0xff, 0xff,
  0xff, 0xcf, 0xcf, 0xff, 0xff,
  0xff, 0xf0, 0x3f, 0xff, 0xff,
};

void WriteScaledPixel(PixelWriter& writer, int scale, int x, int y, const PixelColor& c) {
  for (int px = 0; px < scale; ++px) {
    for (int py = 0; py < scale; ++py) {
      writer.Write(x * scale + px, y * scale + py, c);
    }
  }
}

const PixelColor* ColorFromColorNum2bit(uint8_t colNum) {
  switch (colNum) {
    case 0b00:
      return &BK;
      break;
    case 0b01:
      return &DG;
      break;
    case 0b10:
      return &LG;
      break;
    case 0b11:
      return &WH;
      break;
    default:
      return &BK;
  }
}

uint8_t ColorNum2bitAt(const uint8_t* bmp_data, int x, int y) {
  int i = x + y * AEGIS_WIDTH;
  int data_idx = i / 4;
  int px_idx = i % 4;
  return (bmp_data[data_idx] >> ((3 - px_idx) * 2)) & 0b11;
}

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

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
      const PixelColor* c = ColorFromColorNum2bit(ColorNum2bitAt(AEGIS_BMP_DATA, x, y));
      WriteScaledPixel(*pixel_writer, 4, x, y, *c);
    }
  }

  int i = 0;
  for (char c = '!'; c <= '~'; ++c, ++i) {
    WriteAscii(*pixel_writer, 8 * i, 150, c, WH);
  }
  WriteString(*pixel_writer, 90, 120, "<- Aegis chan", WH);
  WriteString(*pixel_writer, 0, 166, "Hello, world!", WH);

  char buf[128];
  sprintf(buf, "1 + 2 = %d", 1 + 2);
  WriteString(*pixel_writer, 0, 182, buf, WH);

  while (1) __asm__("hlt");
}

