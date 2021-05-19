bits 64
section .text

global SyscallLogString
SyscallLogString:
  mov eax, 0x80000000
  mov r10, rcx
  syscall
  ret

global SyscallPutString
SyscallPutString:
  mov eax, 0x80000001
  mov r10, rcx
  syscall
  ret

global SyscallExit
SyscallExit:
  mov eax, 0x80000002
  mov r10, rcx
  syscall
  ret

global SyscallOpenWindow
SyscallOpenWindow:
  mov eax, 0x80000003
  mov r10, rcx
  syscall
  ret

global SyscallWinWriteString
SyscallWinWriteString:
  mov eax, 0x80000004
  mov r10, rcx
  syscall
  ret

global SyscallWinFillRectangle
SyscallWinFillRectangle:
  mov eax, 0x80000005
  mov r10, rcx
  syscall
  ret

global SyscallGetCurrentTick
SyscallGetCurrentTick:
  mov eax, 0x80000006
  mov r10, rcx
  syscall
  ret

global SyscallWinRedraw
SyscallWinRedraw:
  mov eax, 0x80000007
  mov r10, rcx
  syscall
  ret

global SyscallWinDrawLine
SyscallWinDrawLine:
  mov eax, 0x80000008
  mov r10, rcx
  syscall
  ret

global SyscallCloseWindow
SyscallCloseWindow:
  mov eax, 0x80000009
  mov r10, rcx
  syscall
  ret

global SyscallReadEvent
SyscallReadEvent:
  mov eax, 0x8000000a
  mov r10, rcx
  syscall
  ret

global SyscallCreateTimer
SyscallCreateTimer:
  mov eax, 0x8000000b
  mov r10, rcx
  syscall
  ret
