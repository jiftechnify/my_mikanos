#include "syscall.hpp"

#include <cstdint>
#include <array>
#include <cerrno>
#include <memory>
#include <cmath>
#include <fcntl.h>

#include "asmfunc.h"
#include "msr.hpp"
#include "logger.hpp"
#include "task.hpp"
#include "terminal.hpp"
#include "graphics.hpp"
#include "window.hpp"
#include "layer.hpp"
#include "font.hpp"
#include "timer.hpp"
#include "app_event.hpp"
#include "keyboard.hpp"
#include "fat.hpp"

void InitializeSyscall() {
  WriteMSR(kIA32_EFER, 0x0501u);  // syscall有効化
  WriteMSR(kIA32_LSTAR, reinterpret_cast<uint64_t>(SyscallEntry));  // syscall実行時に呼び出されるOS側の関数のアドレス
  WriteMSR(kIA32_STAR, static_cast<uint64_t>(8) << 32 | 
                       static_cast<uint64_t>(16 | 3) << 48);  // syscall/sysret 時にセグメントレジスタに設定される CS, SS の値を決める
  WriteMSR(kIA32_FMASK, 0);
}

namespace syscall {
  struct Result {
    uint64_t value;
    int error;
  };

#define SYSCALL(name) \
  Result name( \
      uint64_t arg1, uint64_t arg2, uint64_t arg3, \
      uint64_t arg4, uint64_t arg5, uint64_t arg6)

// コンソールに指定ログレベルでログを出力
// arg1: ログレベル
// arg2: 出力文字列
SYSCALL(LogString) {
  if (arg1 != kError && arg1 != kWarn && arg1 != kInfo && arg1 != kDebug) {
    return { 0, EPERM };
  }
  const char* s = reinterpret_cast<const char*>(arg2);
  const auto len = strlen(s);
  if (len > 1024) {
    return { 0, E2BIG };
  }
  Log(static_cast<LogLevel>(arg1), "%s", s);
  return { len, 0 };
}

// 文字列を出力する
// arg1: ファイルディスクリプタ番号
// arg2: 出力文字列へのポインタ
// arg3: 出力文字列のバイト数(\0を含まない)
SYSCALL(PutString) {
  const auto fd = arg1;
  const char* s = reinterpret_cast<const char*>(arg2);
  const auto len = arg3;
  if (len > 1024) {
    return { 0, E2BIG };
  }

  if (fd == 1) { // stdout
    const auto task_id = task_manager->CurrentTask().ID();
    (*terminals)[task_id]->Print(s, len);
    return { len, 0 };
  }
  return { 0, EBADF };
}

// アプリを終了する
// arg1: 終了コード
SYSCALL(Exit) {
  __asm__("cli");
  auto& task = task_manager->CurrentTask();
  __asm__("sti");
  return { task.OSStackPointer(), static_cast<int>(arg1) }; // RAX: OS用スタックポインタ, RDX: 終了コード
}

// ウィンドウを開く
// arg1, arg2: サイズ(w, h)
// arg3, arg4: 位置(x, y)
// arg5: タイトル
SYSCALL(OpenWindow) {
  const int w = arg1, h = arg2, x = arg3, y = arg4;
  const auto title = reinterpret_cast<const char*>(arg5);
  const auto win = std::make_shared<ToplevelWindow>(w, h, screen_config.pixel_format, title);

  __asm__("cli");
  const auto layer_id = layer_manager->NewLayer()
    .SetWindow(win)
    .SetDraggable(true)
    .Move({x, y})
    .ID();
  active_layer->Activate(layer_id);

  const auto task_id = task_manager->CurrentTask().ID();
  layer_task_map->insert(std::make_pair(layer_id, task_id));
  __asm__("sti");

  return { layer_id, 0 };
}

namespace {
  template <class Func, class...Args>
  Result DoWinFunc(Func f, uint64_t layer_id_flags, Args... args) {
    const uint32_t layer_flags = layer_id_flags >> 32;
    const unsigned int layer_id = layer_id_flags & 0xffffffff;

    __asm__("cli");
    auto layer = layer_manager->FindLayer(layer_id);
    __asm__("sti");
    if (layer == nullptr) {
      return {0, EBADF};
    }

    const auto res = f(*layer->GetWindow(), args...);
    if (res.error) {
      return res;
    }

    // 再描画抑止フラグが0なら再描画
    if ((layer_flags & 1) == 0) {
      __asm__("cli");
      layer_manager->Draw(layer_id);
      __asm__("sti");
    }

    return res;
  }
}

// ウィンドウに文字列を出力
// arg1: [0:31]: レイヤID, [32]: 再描画抑止フラグ
// arg2, arg3: 位置(x, y)
// arg4: 色
// arg5: 文字列
SYSCALL(WinWriteString) {
  return DoWinFunc(
      [](Window& win,
         int x, int y, uint32_t color, const char *s) {
        WriteString(*win.Writer(), {x, y}, s, ToColor(color));
        return Result{ 0, 0 };
      }, arg1, arg2,  arg3, arg4, reinterpret_cast<const char*>(arg5));
}

// ウィンドウに矩形を出力
// arg1: [0:31]: レイヤID, [32]: 再描画抑止フラグ
// arg2, arg3: 位置(x, y)
// arg4, arg5: サイズ(w, h)
// arg6: 色
SYSCALL(WinFillRectangle) {
  return DoWinFunc(
      [](Window& win,
         int x, int y, int w, int h, uint32_t color) {
        FillRectangle(*win.Writer(), {x, y}, {w, h}, ToColor(color));
        return Result{ 0, 0 };
      }, arg1, arg2, arg3, arg4, arg5, arg6);
}

// 現在のタイマ値を取得
SYSCALL(GetCurrentTick) {
  return { timer_manager->CurrentTick(), kTimerFreq };
}

// ウィンドウ再描画
// arg1: レイヤID
SYSCALL(WinRedraw) {
  return DoWinFunc(
      [](Window&) {
        return Result{ 0, 0 };
      }, arg1);
}

// ウィンドウに直線を描画
// arg1: レイヤID
// arg2, arg3: 始点(x0, y0)
// arg4, arg5: 終点(x1, y1)
// arg6: 色
SYSCALL(WinDrawLine) {
  return DoWinFunc(
      [](Window& win,
         int x0, int y0, int x1, int y1, uint32_t color) {
        auto sign = [](int x) { 
          return (x > 0) ? 1 :
                 (x < 0) ? -1 : 0;
        };
        const int dx = x1 - x0 + sign(x1 - x0);
        const int dy = y1 - y0 + sign(y1 - y0);

        if (dx == 0 && dy == 0) {
          win.Writer()->Write({x0, y0}, ToColor(color));
          return Result{ 0, 0 };
        }

        const auto floord = static_cast<double(*)(double)>(floor);
        const auto ceild = static_cast<double(*)(double)>(ceil);
        
        if (abs(dx) >= abs(dy)) {
          // 傾き <= 1
          if (dx < 0) {
            std::swap(x0, x1);
            std::swap(y0, y1);
          }
          const auto roundish = y1 >= y0 ? floord : ceild;
          const double m = static_cast<double>(dy) / dx;
          for (int x = x0; x <= x1; ++x) {
            const int y = roundish(m * (x - x0) + y0);
            win.Writer()->Write({x, y}, ToColor(color));
          }
        } else {
          // 傾き > 1
          if (dy < 0) {
            std::swap(x0, x1);
            std::swap(y0, y1);
          }
          const auto roundish = x1 >= x0 ? floord : ceild;
          const double m = static_cast<double>(dx) / dy;
          for (int y = y0; y <= y1; ++y) {
            const int x = roundish(m * (y - y0) + x0);
            win.Writer()->Write({x, y}, ToColor(color));
          }
        }
        return Result{ 0, 0 };
      }, arg1, arg2, arg3, arg4, arg5, arg6);
}

// ウィンドウを閉じる
// arg1: レイヤID
SYSCALL(CloseWindow) {
  const unsigned int layer_id = arg1 & 0xffffffff;
  const auto layer = layer_manager->FindLayer(layer_id);

  if (layer == nullptr) {
    return { EBADF, 0 };
  }

  const auto layer_pos = layer->GetPosition();
  const auto win_size = layer->GetWindow()->Size();

  __asm__("cli");
  active_layer->Activate(0);
  layer_manager->RemoveLayer(layer_id);
  layer_manager->Draw({layer_pos, win_size});
  layer_task_map->erase(layer_id);
  __asm__("sti");

  return { 0, 0 };
}

// アプリに送信されたイベントを取得
// arg1: イベントデータ配列のポインタ
// arg2: イベントデータ配列の長さ
SYSCALL(ReadEvent) {
  if (arg1 < 0x8000'0000'0000'0000) {
    return { 0, EFAULT };
  }
  const auto app_events = reinterpret_cast<AppEvent*>(arg1);
  const size_t len = arg2;

  __asm__("cli");
  auto& task = task_manager->CurrentTask();
  __asm__("sti");
  size_t i = 0;

  while (i < len) {
    __asm__("cli");
    auto msg = task.ReceiveMessage();
    if (!msg && i == 0) {
      // メッセージが届いていなければアプリをスリープさせる
      task.Sleep();
      continue;
    }
    __asm__("sti");

    if (!msg) {
      break;
    }

    switch (msg->type) {
    case Message::kKeyPush:
      if (msg->arg.keyboard.keycode == 20 /* Q key */ &&
          msg->arg.keyboard.modifier & (kLControlBitMask | kRControlBitMask)) {
        app_events[i].type = AppEvent::kQuit;
        ++i;
      } else {
        app_events[i].type = AppEvent::kKeyPush;
        app_events[i].arg.keypush.modifier = msg->arg.keyboard.modifier;
        app_events[i].arg.keypush.keycode = msg->arg.keyboard.keycode;
        app_events[i].arg.keypush.ascii = msg->arg.keyboard.ascii;
        app_events[i].arg.keypush.press = msg->arg.keyboard.press;
        ++i;
      }
      break;
    case Message::kMouseMove:
      app_events[i].type = AppEvent::kMouseMove;
      app_events[i].arg.mouse_move.x = msg->arg.mouse_move.x;
      app_events[i].arg.mouse_move.y = msg->arg.mouse_move.y;
      app_events[i].arg.mouse_move.dx = msg->arg.mouse_move.dx;
      app_events[i].arg.mouse_move.dy = msg->arg.mouse_move.dy;
      app_events[i].arg.mouse_move.buttons = msg->arg.mouse_move.buttons;
      ++i;
      break;
    case Message::kMouseButton:
      app_events[i].type = AppEvent::kMouseButton;
      app_events[i].arg.mouse_button.x = msg->arg.mouse_button.x;
      app_events[i].arg.mouse_button.y = msg->arg.mouse_button.y;
      app_events[i].arg.mouse_button.press = msg->arg.mouse_button.press;
      app_events[i].arg.mouse_button.button = msg->arg.mouse_button.button;
      ++i;
      break;
    case Message::kTimerTimeout:
      if (msg->arg.timer.value < 0) {
        app_events[i].type = AppEvent::kTimerTimeout;
        app_events[i].arg.timer.timeout = msg->arg.timer.timeout;
        app_events[i].arg.timer.value = -msg->arg.timer.value;
        ++i;
      }
      break;
    default:
      Log(kInfo, "uncaught event type: %u\n", msg->type);
    }
  }
  return { i, 0 };
}

// タイマを生成する
// arg1: モード 
// mode[0]: 0: absolute(タイムアウト時刻を指定), 1: relative(タイムアウトまでの時間を指定)
// arg2: タイムアウト時に得られる値
// arg3: タイムアウト時刻 / タイムアウトまでの時間 [msec]
SYSCALL(CreateTimer) {
  const unsigned int mode = arg1;
  const int timer_value = arg2;
  if (timer_value <= 0) {
    return { 0, EINVAL };
  }

  __asm__("cli");
  const uint64_t task_id = task_manager->CurrentTask().ID();
  __asm__("sti");

  unsigned long timeout = arg3 * kTimerFreq / 1000;
  if (mode & 1) { // relative
    timeout += timer_manager->CurrentTick();
  }

  __asm__("cli");
  timer_manager->AddTimer(Timer{timeout, -timer_value, task_id}); // アプリ生成とOS生成を区別するためw、アプリ生成のものはvalueを負に
  __asm__("sti");
  return { timeout * 1000 / kTimerFreq, 0 };
}

namespace {
  size_t AllocateFD(Task& task) {
    const size_t num_files = task.Files().size();
    for (size_t i = 0; i < num_files; ++i) {
      if (!task.Files()[i]) {
        return i;
      }
    }
    task.Files().emplace_back();
    return num_files;
  }
}

// ファイルを開き、ファイルディスクリプタ番号を返す
// arg1: ファイルパス
// arg2: フラグ
SYSCALL(OpenFile) {
  const char* path = reinterpret_cast<const char*>(arg1);
  const int flags = arg2;
  __asm__("cli");
  auto& task = task_manager->CurrentTask();
  __asm__("sti");

  // @stdin が指定された場合は標準入力(fd = 0)を返す
  if (strcmp(path, "@stdin") == 0) {
    return { 0, 0 };
  }

  if ((flags & O_ACCMODE) == O_WRONLY) {
    return { 0, EINVAL };
  }

  auto [ dir, post_slash ] = fat::FindFile(path);
  if (dir == nullptr) {
    return { 0, ENOENT };
  } else if (dir->attr != fat::Attribute::kDirectory && post_slash) {
    return { 0, ENOENT };
  }

  size_t fd = AllocateFD(task);
  task.Files()[fd] = std::make_unique<fat::FileDescriptor>(*dir);
  return { fd, 0 };
}

// ファイルを読み込み、実際に読み込んだバイト数を返す
// arg1: ファイルディスクリプタ番号
// arg2: 読み込みバッファ
// arg3: 読み込むバイト数
SYSCALL(ReadFile) {
  const int fd = arg1;
  void* buf = reinterpret_cast<void*>(arg2);
  size_t count = arg3;

  __asm__("cli");
  auto& task = task_manager->CurrentTask();
  __asm__("sti");

  if (fd < 0 || task.Files().size() <= fd || !task.Files()[fd]) {
    return { 0, EBADF };
  }
  return { task.Files()[fd]->Read(buf, count), 0 };
}

#undef SYSCALL

} // namespace syscall

using SyscallFuncType = syscall::Result (uint64_t, uint64_t, uint64_t,
                                 uint64_t, uint64_t, uint64_t);
extern "C" std::array<SyscallFuncType*, 14> syscall_table{
  /* 0x00 */ syscall::LogString,
  /* 0x01 */ syscall::PutString,
  /* 0x02 */ syscall::Exit,
  /* 0x03 */ syscall::OpenWindow,
  /* 0x04 */ syscall::WinWriteString,
  /* 0x05 */ syscall::WinFillRectangle,
  /* 0x06 */ syscall::GetCurrentTick,
  /* 0x07 */ syscall::WinRedraw,
  /* 0x08 */ syscall::WinDrawLine,
  /* 0x09 */ syscall::CloseWindow,
  /* 0x0a */ syscall::ReadEvent,
  /* 0x0b */ syscall::CreateTimer,
  /* 0x0c */ syscall::OpenFile,
  /* 0x0d */ syscall::ReadFile,
};

