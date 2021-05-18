#ifdef __cplusplus
#include <cstdint>
#include <cstddef>

extern "C" {
#else
#include <stdint.h>
#include <stddef.h>
#endif

#include "../kernel/logger.hpp"

struct SyscallResult {
  uint64_t value;
  int error;
};

struct SyscallResult SyscallLogString(enum LogLevel level, const char* message);
struct SyscallResult SyscallPutString(int fd, const char* s, size_t len);
void SyscallExit(int exit_code);
struct SyscallResult SyscallOpenWindow(int w, int h, int x, int y, const char* title);
struct SyscallResult SyscallWinWriteString(unsigned int layer_id, int x, int y, uint32_t color, const char* s);
struct SyscallResult SyscallWinFillRectangle(unsigned int layer_id, int x, int y, int w, int h, uint32_t color);

#ifdef __cplusplus
}
#endif

