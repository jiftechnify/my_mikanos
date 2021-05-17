#include "syscall.hpp"

#include <cstdint>
#include <array>
#include <cerrno>
#include "asmfunc.h"
#include "msr.hpp"
#include "logger.hpp"
#include "task.hpp"
#include "terminal.hpp"

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

#undef SYSCALL

} // namespace syscall

using SyscallFuncType = syscall::Result (uint64_t, uint64_t, uint64_t,
                                 uint64_t, uint64_t, uint64_t);
extern "C" std::array<SyscallFuncType*, 3> syscall_table{
  /* 0x00 */ syscall::LogString,
  /* 0x01 */ syscall::PutString,
  /* 0x02 */ syscall::Exit,
};

