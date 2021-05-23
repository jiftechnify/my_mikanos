#pragma once

#include <deque>
#include <map>
#include <optional>
#include "window.hpp"
#include "task.hpp"
#include "layer.hpp"
#include "fat.hpp"
#include "file.hpp"

class Terminal {
  public:
    static const int kRows = 15, kColumns = 60;
    static const int kLineMax = 128;

    Terminal(uint64_t task_id, bool show_window);
    unsigned int LayerID() const { return layer_id_; }
    Rectangle<int> BlinkCursor();
    Rectangle<int> InputKey(uint8_t modifier, uint8_t keycode, char ascii);
    void Print(char c);
    void Print(const char* s, std::optional<size_t> len = std::nullopt);
    void ExecuteLine();
    Error ExecuteFile(const fat::DirectoryEntry& entry, char* command, char* first_arg);

  private:
    std::shared_ptr<ToplevelWindow> window_;
    unsigned int layer_id_;
    uint64_t task_id_;
    bool show_window_;

    Vector2D<int> cursor_{0, 0};
    bool cursor_visible_{false};
    void DrawCursor(bool visible);
    Vector2D<int> CalcCursorPos() const;

    int linebuf_index_{0};
    std::array<char, kLineMax> linebuf_{};
    void Scroll1();

    // コマンド履歴用
    std::deque<std::array<char, kLineMax>> cmd_history_{};
    int cmd_history_index_{-1};
    Rectangle<int> HistoryUpDown(int direction);
};

// タスクIDからターミナルインスタンスを取得するためのMap
extern std::map<uint64_t, Terminal*>* terminals;

void TaskTerminal(uint64_t task_id, int64_t data);

class TerminalFileDescriptor : public FileDescriptor {
  public:
    explicit TerminalFileDescriptor(Task& task, Terminal& term);
    size_t Read(void* buf, size_t len) override;

  private:
    Task& task_;
    Terminal& term_;
};
