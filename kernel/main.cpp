#include <cstdint>
#include <cstddef>
#include <cstdio>
#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

void* operator new(size_t size, void* buf) {
  return buf;
}

void operator delete(void* obj) noexcept {
}

const PixelColor BK{0, 0, 0};
const PixelColor DG{102, 102, 102};
const PixelColor LG{187, 187, 187};
const PixelColor WH{255, 255, 255};
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

void WriteScaledPixel(PixelWriter& writer, int scale, int left_x, int top_y, int px, int py, const PixelColor& c) {
  for (int dx = 0; dx < scale; ++dx) {
    for (int dy = 0; dy < scale; ++dy) {
      writer.Write(left_x + px*scale + dx, top_y + py*scale + dy, c);
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

const int kMouseCursorWidth = 15;
const int kMouseCursorHeight = 24;
const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
  "@              ",
  "@@             ",
  "@.@            ",
  "@..@           ",
  "@...@          ",
  "@....@         ",
  "@.....@        ",
  "@......@       ",
  "@.......@      ",
  "@........@     ",
  "@.........@    ",
  "@..........@   ",
  "@...........@  ",
  "@............@ ",
  "@......@@@@@@@@",
  "@......@       ",
  "@....@@.@      ",
  "@...@ @.@      ",
  "@..@   @.@     ",
  "@.@    @.@     ",
  "@@      @.@    ",
  "@       @.@    ",
  "         @.@   ",
  "         @@@   ",
};

char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

char console_buf[sizeof(Console)];
Console* console;

int printk(const char* format, ...) {
  va_list ap;
  int result;
  char s[1024];

  va_start(ap, format);
  result = vsprintf(s, format, ap);
  va_end(ap);

  console->PutString(s);
  return result;
}

const PixelColor kDesktopBGColor{45, 118, 237};
const PixelColor kDesktopFGColor{255, 255, 255};

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
  console = new(console_buf) Console{*pixel_writer, kDesktopFGColor, kDesktopBGColor};

  const int kFrameWidth = frame_buffer_config.horizontal_resolution;
  const int kFrameHeight = frame_buffer_config.vertical_resolution;

  FillRectangle(*pixel_writer, {0, 0}, {kFrameWidth, kFrameHeight - 50}, kDesktopBGColor);
  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth, 50}, {1, 8, 17});
  FillRectangle(*pixel_writer, {0, kFrameHeight - 50}, {kFrameWidth / 5, 50}, {80, 80, 80});
  DrawRectangle(*pixel_writer, {10, kFrameHeight - 40}, {30, 30}, {160, 160, 160});

  printk("Welcome to Mikan OS!\n");

  for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
    for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
      if (mouse_cursor_shape[dy][dx] == '@') {
        pixel_writer->Write(200 + dx, 100 + dy, BK);
      } else if (mouse_cursor_shape[dy][dx] == '.') {
        pixel_writer->Write(200 + dx, 100 + dy, WH);
      }
    }
  }
  for (int px = 0; px < AEGIS_WIDTH; ++px) {
    for (int py = 0; py < AEGIS_HEIGHT; ++py) {
      const PixelColor* c = ColorFromColorNum2bit(ColorNum2bitAt(AEGIS_BMP_DATA, px, py));
      WriteScaledPixel(*pixel_writer, 4, 0, 400, px, py, *c);
    }
  }
  WriteString(*pixel_writer, 90, 520, "<- Aegis chan", BK);
 

  while (1) __asm__("hlt");
}

