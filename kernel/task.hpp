#pragma once

#include <array>
#include <cstdint>

// タスクコンテキストの保存先
struct TaskContext {
  uint64_t cr3, rip, rflags, reserved1;             // 0x00
  uint64_t cs, ss, fs, gs;                          // 0x20
  uint64_t rax, rbx ,rcx, rdx, rdi, rsi, rsp, rbp;  // 0x40
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;    // 0x80
  std::array<uint8_t, 512> fxsave_area;             // 0xc0
} __attribute__((packed));

extern TaskContext task_b_ctx, task_a_ctx;

void InitializeTask();
void SwitchTask();

