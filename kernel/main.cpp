#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>
#include <deque>
#include <limits>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "mouse.hpp"
#include "asmfunc.h"
#include "interrupt.hpp"
#include "queue.hpp"
#include "memory_map.hpp"
#include "segment.hpp"
#include "paging.hpp"
#include "layer.hpp"
#include "window.hpp"
#include "memory_manager.hpp"
#include "timer.hpp"
#include "grayscale_image.hpp"
#include "message.hpp"
#include "usb/xhci/xhci.hpp"

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

std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;
void InitializeMainWindow() {
  main_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
  DrawWindow(*main_window->Writer(), "Hello Window");

  main_window_layer_id = layer_manager->NewLayer()
    .SetWindow(main_window)
    .SetDraggable(true)
    .Move({300, 100})
    .ID();

  layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

std::shared_ptr<Window> aegis_window;
unsigned int aegis_window_layer_id;
void InitializeAegisWindow() {
  aegis_window = std::make_shared<Window>(92, 164, screen_config.pixel_format);
  DrawWindow(*aegis_window->Writer(), "Aegis");
  DrawGrayscale4GradsImageScaled(*aegis_window->Writer(), {6, 24}, 4, GrayscaleAegis);
  
  aegis_window_layer_id = layer_manager->NewLayer()
    .SetWindow(aegis_window)
    .SetDraggable(true)
    .Move({250, 200})
    .ID();

  layer_manager->UpDown(aegis_window_layer_id, std::numeric_limits<int>::max());
}

std::deque<Message>* main_queue;

// カーネルが利用するスタック領域を準備
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

// メモリマネージャ
char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager* memory_manager;


extern "C" void KernelMainNewStack(const FrameBufferConfig& frame_buffer_config_ref, const MemoryMap& memory_map_ref) {
  MemoryMap memory_map{memory_map_ref};

  InitializeGraphics(frame_buffer_config_ref);
  InitializeConsole();

  printk("Welcome to MikanOS!\n");
  SetLogLevel(kWarn);

  InitializeLAPICTimer();

  // セグメンテーションの設定
  InitializeSegmentation();

  // ページングの設定(とりあえずアイデンティティマッピング)
  InitializePaging();

  // メモリマネージャに利用可能なメモリ領域の情報を伝える
  InitializeMemoryManager(memory_map);
  
  ::main_queue = new std::deque<Message>(32);
  InitializeInterrupt(main_queue);

  InitializePCI();
  usb::xhci::Initialize();

  InitializeLayer();
  InitializeMainWindow();
  InitializeAegisWindow();
  InitializeMouse();

  layer_manager->DrawAll();

  char str[128];
  unsigned int count = 0;

  // 割り込みハンドラからのメッセージを処理するイベントループ
  while (true) {
    ++count;
    sprintf(str, "%010u", count);
    FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
    WriteString(*main_window->Writer(), {24, 28}, str, {0, 0, 0});
    layer_manager->Draw(main_window_layer_id);

    // cli 命令でキュー操作中に割り込みイベントを受け取らないようにする
    // キュー操作が終わり次第、sti 命令で割り込みイベントを受け取るようにする
    __asm__("cli");
    if (main_queue->size() == 0) {
      __asm__("sti");
      continue;
    }

    Message msg = main_queue->front();
    main_queue->pop_front();
    __asm__("sti");

    switch (msg.type) {
    case Message::kInterruptXHCI:
      usb::xhci::ProcessEvents();
      break;
    default:
      Log(kError, "Unknown message type: %d\n", msg.type);
    }
  }
  while (1) __asm__("hlt");
}

extern "C" void __cxa_pure_virtual() {
  while (1) __asm__("hlt");
}
