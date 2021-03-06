#pragma once

#include "error.hpp"
#include "message.hpp"
#include "file.hpp"
#include <array>
#include <vector>
#include <memory>
#include <queue>
#include <optional>
#include <cstdint>
#include <cstddef>
#include <map>

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

struct FileMapping;
class TaskManager;

// タスクを表すクラス
class Task {
  public:
    static const int kDefaultLevel = 1;
    static const size_t kDefaultStackBytes = 8 * 4096;
    Task(uint64_t id);
    Task& InitContext(TaskFunc* f, int64_t data);
    TaskContext& Context();
    uint64_t& OSStackPointer();

    uint64_t ID() const;
    Task& Sleep();
    Task& Wakeup();

    void SendMessage(const Message& msg);
    std::optional<Message> ReceiveMessage();

    int Level() const { return level_; }
    bool Running() const { return running_; }

    std::vector<std::shared_ptr<::FileDescriptor>>& Files();

    uint64_t DPagingBegin() const;
    void SetDPagingBegin(uint64_t v);
    uint64_t DPagingEnd() const;
    void SetDPagingEnd(uint64_t v);

    uint64_t FileMapEnd() const;
    void SetFileMapEnd(uint64_t v);
    std::vector<FileMapping>& FileMaps();

  private:
    uint64_t id_;
    std::vector<uint64_t> stack_;
    alignas(16) TaskContext context_;
    uint64_t os_stack_ptr_;
    std::deque<Message> msgs_;  // 受け取ったメッセージを収めるキュー
    unsigned int level_{kDefaultLevel};
    bool running_{false};

    std::vector<std::shared_ptr<::FileDescriptor>> files_{};

    uint64_t dpaging_begin_{0}, dpaging_end_{0};
    uint64_t file_map_end_{0};
    std::vector<FileMapping> file_maps_{};

    Task& SetLevel(int level) { level_ = level; return *this; }
    Task& SetRunning(bool running) { running_ = running; return *this; }

    friend TaskManager; // TaskManager にのみ privateメソッドの呼び出しを許可
};

// 複数のタスクを管理するクラス
class TaskManager {
  public:
    static const int kMaxLevel = 3;

    TaskManager();
    Task& NewTask();
    void SwitchTask(const TaskContext& current_ctx);
    Task& CurrentTask();
    
    void Sleep(Task* task);
    Error Sleep(uint64_t id);
    void Wakeup(Task* task, int level = -1);
    Error Wakeup(uint64_t id, int level = -1);

    Error SendMessage(uint64_t id, const Message& msg);

    WithError<int> WaitFinish(uint64_t task_id);
    void Finish(int exit_code); // 呼び出したタスクを指定終了コードで終了させる

  private:
    std::vector<std::unique_ptr<Task>> tasks_{};
    uint64_t latest_id_{0};
    std::array<std::deque<Task*>, kMaxLevel + 1> running_{}; // 実行可能状態のタスクを並べるキュー(優先度レベル別)
    int current_level_{kMaxLevel};
    bool level_changed_{false};
    std::map<uint64_t, int> finish_tasks_{};    // 終了したタスクの終了コードを記録
    std::map<uint64_t, Task*> finish_waiter_{}; // タスクと、そのタスクの終了を待っているタスクの対応づけ

    void ChangeLevelRunning(Task* task, int level);
    Task* RotateCurrentRunQueue(bool current_sleep);
};

extern TaskManager* task_manager;

// ファイルマッピングを表現する
// ファイルが仮想アドレスのどの範囲にマップされているかの情報を持つ
struct FileMapping {
  int fd;
  uint64_t vaddr_begin, vaddr_end;
};

