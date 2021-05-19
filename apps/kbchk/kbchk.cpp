#include <cstdio>
#include <cstdlib>
#include "../syscall.h"
#include "../../kernel/keyboard.hpp"

static const int kWidth = 64, kHeight = 16;
static const int kHMargin = 8, kVMargin = 4;

extern "C" void main(int argc, char** argv) {
  auto [layer_id, err_openwin] = SyscallOpenWindow(88, 52, 300, 50, "kbchk");
  if (err_openwin) {
    printf("failed to open window: %s\n", strerror(err_openwin));
    exit(err_openwin);
  }

  SyscallWinFillRectangle(layer_id, 4, 24, kWidth + kHMargin * 2, kHeight + kVMargin * 2, 0x000000);

  AppEvent events[1];
  bool prev_press = false;

  while (true) {
    auto [n, err] = SyscallReadEvent(events, 1);
    if (err) {
      printf("failed to read event: %s\n", strerror(err));
      break;
    }
    if (events[0].type == AppEvent::kQuit) {
      printf("bye\n");
      break;
    }
    if (events[0].type == AppEvent::kKeyPush) {
      auto& arg = events[0].arg.keypush;
      if (!arg.press) {
        // if (prev_press) {
        //   SyscallWinFillRectangle(layer_id, 4 + kHMargin, 24 + kVMargin, kWidth, kHeight, 0x000000);
        //   prev_press = false;
        // }
        continue;
      }
    
      char s[9];
      for (int i = 0; i < 8; ++i) {
        s[i] = ' ';
      }
      if (arg.modifier & (kLControlBitMask | kRControlBitMask)) {
        s[0] = 'C';
      }
      if (arg.modifier & (kLShiftBitMask | kRShiftBitMask)) {
        s[1] = 'S';
      }
      if (arg.modifier & (kLAltBitMask | kRAltBitMask)) {
        s[2] = 'A';
      }
      if (arg.modifier & (kLGUIBitMask | kRGUIBitMask)) {
        s[3] = 'M';
      }
      if (arg.modifier) {
        s[5] = '+';
      }
      s[7] = arg.ascii;

      SyscallWinFillRectangle(layer_id | LAYER_NO_REDRAW, 4 + kHMargin, 24 + kVMargin, kWidth, kHeight, 0x000000);
      SyscallWinWriteString(layer_id, 4 + kHMargin, 24 + kVMargin, 0xffffff, s);

      prev_press = true;
    } else {
      printf("unknown event: type = %d\n", events[0].type);
    }
  }

  SyscallCloseWindow(layer_id);
  exit(0);
}
