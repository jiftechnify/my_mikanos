#pragma once

#include "error.hpp"
#include "message.hpp"
#include <array>
#include <vector>
#include <memory>
#include <queue>
#include <optional>
#include <cstdint>
#include <cstddef>

// タスクコンテキストの保存先
struct TaskContext {
  uint64_t cr3, rip, rflags, reserved1;             // 0x00
  uint64_t cs, ss, fs, gs;                          // 0x20
  uint64_t rax, rbx ,rcx, rdx, rdi, rsi, rsp, rbp;  // 0x40
  uint64_t r8, r9, r10, r11, r12, r13, r14, r15;    // 0x80
  std::array<uint8_t, 512> fxsave_area;             // 0xc0
} __attribute__((packed));

void InitializeTask();
void SwitchTask();

// タスクが実行する関数の型
using TaskFunc = void (uint64_t, int64_t);

// タスクを表すクラス
class Task {
  public:
    static const size_t kDefaultStackBytes = 4096;
    Task(uint64_t id);
    Task& InitContext(TaskFunc* f, int64_t data);
    TaskContext& Context();

    uint64_t ID() const;
    Task& Sleep();
    Task& Wakeup();

    void SendMessage(const Message& msg);
    std::optional<Message> ReceiveMessage();

  private:
    uint64_t id_;
    std::vector<uint64_t> stack_;
    alignas(16) TaskContext context_;
    std::deque<Message> msgs_;  // 受け取ったメッセージを収めるキュー
};

// 複数のタスクを管理するクラス
class TaskManager {
  public:
    TaskManager();
    Task& NewTask();
    void SwitchTask(bool current_sleep = false);
    Task& CurrentTask();
    
    void Sleep(Task* task);
    Error Sleep(uint64_t id);
    void Wakeup(Task* task);
    Error Wakeup(uint64_t id);

    Error SendMessage(uint64_t id, const Message& msg);

  private:
    std::vector<std::unique_ptr<Task>> tasks_{};
    uint64_t latest_id_{0};
    std::deque<Task*> running_{}; // 実行可能状態のタスクを並べるキュー
};

extern TaskManager* task_manager;

