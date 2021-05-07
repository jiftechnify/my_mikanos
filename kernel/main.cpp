#include <cstdint>
#include <cstddef>
#include <cstdio>
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
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"

// const PixelColor BK{0, 0, 0};
// const PixelColor DG{102, 102, 102};
// const PixelColor LG{187, 187, 187};
// const PixelColor WH{255, 255, 255};
// const int AEGIS_WIDTH = 20;
// const int AEGIS_HEIGHT = 34;
// const int AEGIS_BMP_DATA_SIZE = (AEGIS_WIDTH * AEGIS_HEIGHT + 3) / 4;
// const uint8_t AEGIS_BMP_DATA[AEGIS_BMP_DATA_SIZE] = {
//   0xff, 0xfc, 0x00, 0x3f, 0xff,
//   0xff, 0xc1, 0x55, 0x43, 0xff,
//   0xff, 0x16, 0xaa, 0x94, 0xff,
//   0xfc, 0x6a, 0xaa, 0xa9, 0x3f,
//   0xf1, 0xaa, 0xaa, 0xaa, 0x4f,
//   0xf1, 0xaa, 0xaa, 0xaa, 0x4f,
//   0xc6, 0xbe, 0xaa, 0xaa, 0x93,
//   0xc6, 0x80, 0xeb, 0x02, 0x93,
//   0x16, 0x30, 0xfb, 0x0c, 0x94,
//   0x16, 0xfc, 0xff, 0xcf, 0x94,
//   0x16, 0xf0, 0xff, 0x0f, 0x94,
//   0xc6, 0xff, 0xcf, 0xff, 0x93,
//   0xf2, 0x3f, 0xff, 0xfc, 0x8f,
//   0xfc, 0x8f, 0xc3, 0xf2, 0x3f,
//   0xff, 0x00, 0x3c, 0x00, 0xff,
//   0xff, 0x30, 0x00, 0x03, 0xff,
//   0xff, 0xc0, 0x00, 0x3c, 0xff,
//   0xff, 0xfc, 0x00, 0x0f, 0x0f,
//   0xff, 0xfc, 0x00, 0x33, 0xf3,
//   0xff, 0xf0, 0x00, 0x0c, 0xf3,
//   0xff, 0xc0, 0x00, 0x03, 0x0f,
//   0xff, 0x00, 0x00, 0x00, 0xff,
//   0xfc, 0x00, 0x00, 0x00, 0x3f,
//   0xf3, 0x00, 0x00, 0x00, 0xcf,
//   0xfc, 0xf0, 0x00, 0x0f, 0x3f,
//   0xff, 0x0f, 0xff, 0xf0, 0xff,
//   0xff, 0xf0, 0x00, 0x0f, 0xff,
//   0xff, 0xf3, 0xc7, 0xcf, 0xff,
//   0xff, 0xf1, 0x45, 0xcf, 0xff,
//   0xff, 0xf1, 0x41, 0x4f, 0xff,
//   0xff, 0xf1, 0x4c, 0x3f, 0xff,
//   0xff, 0xc5, 0x4f, 0xff, 0xff,
//   0xff, 0xcf, 0xcf, 0xff, 0xff,
//   0xff, 0xf0, 0x3f, 0xff, 0xff,
// };
// 
// void WriteScaledPixel(PixelWriter& writer, int scale, Vector2D<int> pos, Vector2D<int> pixel_at, const PixelColor& c) {
//   for (int dx = 0; dx < scale; ++dx) {
//     for (int dy = 0; dy < scale; ++dy) {
//       writer.Write(pos.x + pixel_at.x*scale + dx, pos.y + pixel_at.y*scale + dy, c);
//     }
//   }
// }
// 
// const PixelColor* ColorFromColorNum2bit(uint8_t colNum) {
//   switch (colNum) {
//     case 0b00:
//       return &BK;
//       break;
//     case 0b01:
//       return &DG;
//       break;
//     case 0b10:
//       return &LG;
//       break;
//     case 0b11:
//       return &WH;
//       break;
//     default:
//       return &BK;
//   }
// }
// 
// uint8_t ColorNum2bitAt(const uint8_t* bmp_data, Vector2D<int> at) {
//   int i = at.x + at.y * AEGIS_WIDTH;
//   int data_idx = i / 4;
//   int px_idx = i % 4;
//   return (bmp_data[data_idx] >> ((3 - px_idx) * 2)) & 0b11;
// }


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

// Intel製のUSB HCIのモードをEHCIモード(USB2.0)からxHCIモード(USB3.x)に切り替え
void SwitchEhci2Xhci(const pci::Device& xhc_dev) {
  bool intel_ehc_exist = false;
  for (int i = 0; i < pci::num_device; ++i) {
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) /* EHCI */ &&
          0x8086 == pci::ReadVendorId(pci::devices[i])) {
      intel_ehc_exist = true;
      break;
    }
  }
  if (!intel_ehc_exist) {
    return;
  }

  uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc);  // USB3PRM
  pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);  // USB3_PSSEN
  uint32_t ehci2xhci_ports = pci::ReadConfReg(xhc_dev, 0xd4); // XUSB2PRM
  pci::WriteConfReg(xhc_dev, 0xd0, ehci2xhci_ports);  // XUSB2PR
  Log(kDebug, "SwitchEhci2Xhci: SS = %02x, xHCI = %02x\n",
      superspeed_ports, ehci2xhci_ports);
}

struct Message {
  enum Type {
    kInterruptXHCI,
  } type;
};

ArrayQueue<Message>* main_queue;

usb::xhci::Controller* xhc;

// xHCI用割り込みハンドラ
__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
  main_queue->Push(Message{Message::kInterruptXHCI});
  NotifyEndOfInterrupt();
}

unsigned int mouse_layer_id;
Vector2D<int> screen_size;
Vector2D<int> mouse_position;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
  auto newpos = mouse_position + Vector2D<int>{displacement_x, displacement_y};
  newpos = ElementMin(newpos, screen_size + Vector2D<int>{-1, -1});
  mouse_position = ElementMax(newpos, Vector2D<int>{0, 0});

  layer_manager->Move(mouse_layer_id, mouse_position);
}

// カーネルが利用するスタック領域を準備
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

// メモリマネージャ
char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager* memory_manager;

extern "C" void KernelMainNewStack(const FrameBufferConfig& frame_buffer_config_ref, const MemoryMap& memory_map_ref) {
  FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
  MemoryMap memory_map{memory_map_ref};

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
  
  // レイヤマネージャが用意できるまでの臨時コンソール
  console->SetWriter(pixel_writer);
  DrawDesktop(*pixel_writer);

  console = new(console_buf) Console{kDesktopFGColor, kDesktopBGColor};
  console->SetWriter(pixel_writer);
  printk("Welcome to Mikan OS!\n");
  SetLogLevel(kWarn);

  InitializeLAPICTimer();

  // セグメンテーションの設定
  SetupSegments();

  const uint16_t kernel_cs = 1 << 3;
  const uint16_t kernel_ss = 2 << 3;
  SetDSAll(0);
  SetCSSS(kernel_cs, kernel_ss);

  // ページングの設定(とりあえずアイデンティティマッピング)
  SetupIdentityPageTable();

  // メモリマネージャに利用可能なメモリ領域の情報を伝える
  ::memory_manager = new(memory_manager_buf) BitmapMemoryManager;

  const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
  uintptr_t available_end = 0;
  for (uintptr_t iter = memory_map_base;
       iter < memory_map_base + memory_map.map_size;
       iter += memory_map.descriptor_size) {
    auto desc = reinterpret_cast<const MemoryDescriptor*>(iter);

    // メモリマップ上にない領域は使用済みとマーク
    if (available_end < desc->physical_start) {
      memory_manager->MarkAllocated(
          FrameID{available_end / kBytesPerFrame},
          (desc->physical_start - available_end) / kBytesPerFrame);
    }

    const auto physical_end =
      desc->physical_start + desc->number_of_pages * kUEFIPageSize;
    if (IsAvailable(static_cast<MemoryType>(desc->type))) {
      available_end = physical_end;
    } else {
      // カーネルで利用できない領域は使用済みとマーク
      memory_manager->MarkAllocated(
          FrameID{desc->physical_start / kBytesPerFrame},
          desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
    }   
  }
  memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});
  
  // malloc で動的に確保するためのメモリ領域を予約
  if (auto err = InitializeHeap(*memory_manager)) {
    Log(kError, "failed to allocate pages: %s at %s:%d\n",
        err.Name(), err.File(), err.Line());
    exit(1);
  }

  std::array<Message, 32> main_queue_data;
  ArrayQueue<Message> main_queue{main_queue_data};
  ::main_queue = &main_queue;

  // for (int px = 0; px < AEGIS_WIDTH; ++px) {
  //   for (int py = 0; py < AEGIS_HEIGHT; ++py) {
  //     const PixelColor* c = ColorFromColorNum2bit(ColorNum2bitAt(AEGIS_BMP_DATA, {px, py}));
  //     WriteScaledPixel(*pixel_writer, 4, {0, 400}, {px, py}, *c);
  //   }
  // }
  // WriteString(*pixel_writer, 90, 520, "<- Aegis chan", BK);

  // xHCを探す
  pci::ScanAllBus();
  pci::Device* xhc_dev = nullptr;
  for (int i = 0; i < pci::num_device; ++i) {
    // base      = 0x0c: シリアルバスコントローラ
    // sub       = 0x03: USBコントローラ
    // interface = 0x30: xHCI
    if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
      xhc_dev = &pci::devices[i];
      
      // Vendor ID = 0x8086: Intel製
      if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
        break;
      }
    }
  }
  if (xhc_dev) {
    Log(kInfo, "xHC has been found: %d.%d.%d\n",
        xhc_dev->bus, xhc_dev->device, xhc_dev->function);
  }

  // xHCI用割り込みハンドラを登録
  const uint16_t cs = GetCS();
  SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0),
      reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
  LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

  /// Local APIC ID: CPUコア毎に固有の番号 割り込みがどのコアに通知されるかを指定するために取得
  const uint8_t bsp_local_apic_id =
    *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
  // MSI割り込みの有効化
  pci::ConfigureMSIFixedDestination(
      *xhc_dev, bsp_local_apic_id,
      pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed,
      InterruptVector::kXHCI, 0);

  // xHCを制御するレジスタのMMIOアドレスを読み出す
  // アドレスは BAR0 に書いてある
  const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_dev, 0);
  Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
  const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf); // 64ビットのうち下位4ビットをマスク
  Log(kDebug, "xHC mmio_base: %08lx\n", xhc_mmio_base);
  
  // xHCの初期化と起動
  usb::xhci::Controller xhc{xhc_mmio_base};

  if (0x8086 == pci::ReadVendorId(*xhc_dev)) {
    SwitchEhci2Xhci(*xhc_dev);
  }
  {
    auto err = xhc.Initialize();
    Log(kDebug, "xhc.Initialize: %s\n", err.Name());
  }
  Log(kInfo, "xHC starting\n");
  xhc.Run();

  // 機器が接続されているUSBポートに対し、設定を行う
  usb::HIDMouseDriver::default_observer = MouseObserver;

  for (int i = 1; i < xhc.MaxPorts(); ++i) {
    auto port = xhc.PortAt(i);
    Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

    if (port.IsConnected()) {
      if (auto err = ConfigurePort(xhc, port)) {
        Log(kError, "failed to configure port: %s at %s:%d\n",
            err.Name(), err.File(), err.Line());
        continue;
      }
    }
  }

  // レイヤマネージャ初期化
  // 背景とマウスカーソルの2レイヤからなる画面を描画
  screen_size.x = frame_buffer_config.horizontal_resolution; 
  screen_size.y = frame_buffer_config.vertical_resolution;

  auto bgwindow = std::make_shared<Window>(screen_size.x, screen_size.y, frame_buffer_config.pixel_format);
  auto bgwriter = bgwindow->Writer();
  DrawDesktop(*bgwriter);

  auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight, frame_buffer_config.pixel_format);
  mouse_window->SetTransparentColor(kMouseTransparentColor);
  DrawMouseCursor(mouse_window->Writer(), {0, 0});

  auto main_window = std::make_shared<Window>(160, 52, frame_buffer_config.pixel_format);
  DrawWindow(*main_window->Writer(), "Hello Window");

  auto console_window = std::make_shared<Window>(Console::kColumns * 8, Console::kRows * 16, frame_buffer_config.pixel_format);
  console->SetWindow(console_window);

  FrameBuffer screen;
  if (auto err = screen.Initialize(frame_buffer_config)) {
    Log(kError, "failed to intialize frame buffer: %s at %s:%d\n",
        err.Name(), err.File(), err.Line());
  }

  layer_manager = new LayerManager;
  layer_manager->SetWriter(&screen);

  auto bglayer_id = layer_manager->NewLayer()
    .SetWindow(bgwindow)
    .Move({0, 0})
    .ID();

  mouse_layer_id = layer_manager->NewLayer()
    .SetWindow(mouse_window)
    .Move({200, 200})
    .ID();

  auto main_window_layer_id = layer_manager->NewLayer()
    .SetWindow(main_window)
    .Move({300, 100})
    .ID();

  console->SetLayerID(layer_manager->NewLayer()
    .SetWindow(console_window)
    .Move({0, 0})
    .ID());

  layer_manager->UpDown(bglayer_id, 0);
  layer_manager->UpDown(console->LayerID(), 1);
  layer_manager->UpDown(main_window_layer_id, 2);
  layer_manager->UpDown(mouse_layer_id, 3);
  layer_manager->Draw();

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
    if (main_queue.Count() == 0) {
      __asm__("sti");
      continue;
    }

    Message msg = main_queue.Front();
    main_queue.Pop();
    __asm__("sti");

    switch (msg.type) {
    case Message::kInterruptXHCI:
    
      while (xhc.PrimaryEventRing()->HasFront()) {
        if (auto err = ProcessEvent(xhc)) {
          Log(kError, "Error while Process Event: %s at %s:%d\n",
              err.Name(), err.File(), err.Line());
        }
      }
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
