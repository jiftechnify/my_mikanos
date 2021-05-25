#pragma once

#include <deque>
#include <map>
#include <optional>
#include <memory>
#include "window.hpp"
#include "task.hpp"
#include "layer.hpp"
#include "fat.hpp"
#include "file.hpp"
#include "paging.hpp"

// ターミナルの起動設定を格納する構造体
struct TerminalDescriptor {
  std::string command_line;
  bool exit_after_command;
  bool show_window;
  std::array<std::shared_ptr<FileDescriptor>, 3> files;
};

class Terminal {
  public:
    static const int kRows = 15, kColumns = 60;
    static const int kLineMax = 128;

    Terminal(Task& task, const TerminalDescriptor* term_desc);
    unsigned int LayerID() const { return layer_id_; }
    Rectangle<int> BlinkCursor();
    Rectangle<int> InputKey(uint8_t modifier, uint8_t keycode, char ascii);
    void Print(char32_t c);
    void Print(const char* s, std::optional<size_t> len = std::nullopt);
    void Redraw();

    void ExecuteLine();
    WithError<int> ExecuteFile(fat::DirectoryEntry& entry, char* command, char* first_arg);
    int LastExitCode() const { return last_exit_code_; }

    Task& UnderlyingTask() const { return task_; }

  private:
    std::shared_ptr<ToplevelWindow> window_;
    unsigned int layer_id_;
    Task& task_;
    bool show_window_;
    std::array<std::shared_ptr<FileDescriptor>, 3> files_;  // stdin/out/err
    int last_exit_code_{0};

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

// ファイルの一種としてターミナルを利用するためのFileDescriptor実装
class TerminalFileDescriptor : public FileDescriptor {
  public:
    explicit TerminalFileDescriptor(Terminal& term);
    size_t Read(void* buf, size_t len) override;
    size_t Write(const void* buf, size_t len) override;
    size_t Size() const override { return 0; }
    size_t Load(void*buf, size_t len, size_t offset) override;

  private:
    Terminal& term_;
};

// アプリのELFをメモリにロードした状態を記憶する構造体
// 同じアプリを複数起動する際、ELFをロードする代わりにこれをコピーすることで起動コストを削減する
struct AppLoadInfo {
  uint64_t vaddr_end; // LOADセグメントの終点(= デマンドページングの始点)
  uint64_t entry;     // エントリポイント
  PageMapEntry* pml4; // このアプリのページテーブルの起点
};

// アプリをキーとするAppLoadInfoのmap
extern std::map<fat::DirectoryEntry*, AppLoadInfo>* app_loads;

// パイプ 読み書きができる対象なので FileDescriptor の一種
class PipeDescriptor : public FileDescriptor {
  public:
    explicit PipeDescriptor(Task& task);
    size_t Read(void* buf, size_t len) override;
    size_t Write(const void* buf, size_t len) override;
    size_t Size() const override { return 0; }
    size_t Load(void*buf, size_t len, size_t offset) override { return 0; }

    void FinishWrite();

  private:
    Task& task_;
    char data_[16];
    size_t len_{0};
    bool closed_{false};
};
