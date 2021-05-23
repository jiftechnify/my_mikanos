#pragma once

#include <array>
#include <limits>

#include "error.hpp"
#include "memory_map.hpp"

// ユーザ定義リテラルを利用してメモリサイズを単位付きのリテラルで表現できるようにする
namespace {
  constexpr unsigned long long operator""_KiB(unsigned long long kib) {
    return kib * 1024;
  }

  constexpr unsigned long long operator""_MiB(unsigned long long mib) {
    return mib * 1024_KiB;
  }

  constexpr unsigned long long operator""_GiB(unsigned long long gib) {
    return gib * 1024_MiB;
  }
}

// ページフレーム1つあたりの大きさ
static const auto kBytesPerFrame{4_KiB};

// ページフレーム番号を表す型
class FrameID {
  public:
    explicit FrameID(size_t id) : id_{id} {}
    size_t ID() const { return id_; }
    void* Frame() const { return reinterpret_cast<void*>(id_ * kBytesPerFrame); }

  private:
    size_t id_;
};

// 未定義ページフレームを指すフレーム番号
static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};

// メモリ使用状態を表す
struct MemoryStat {
  size_t allocated_frames;
  size_t total_frames;
};

class BitmapMemoryManager {
  public:
    // 扱える最大物理メモリサイズ
    static const auto kMaxPhysicalMemoryBytes{128_GiB};
    // 最大サイズの物理メモリを扱うのに必要なフレーム数
    static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};

    using MapLineType = unsigned long;
    // ビットマップ配列の1要素が表すフレーム数(1bit = 1フレームなので、要素型のバイト数 * 8)
    static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};

    BitmapMemoryManager();

    // 指定フレーム数のメモリ領域を確保
    WithError<FrameID> Allocate(size_t num_frames);
    // 指定フレーム範囲のメモリを解放
    Error Free(FrameID start_frame, size_t num_frames);
    // 使用済み領域をマーク
    void MarkAllocated(FrameID start_frame, size_t num_frames);

    // メモリマネージャで扱うメモリ範囲を設定(この範囲外のメモリはAllocateにより割り当てない)
    void SetMemoryRange(FrameID range_begin, FrameID range_end);

    // メモリの使用状態を取得
    MemoryStat Stat() const;

  private:
    std::array<MapLineType, kFrameCount / kBitsPerMapLine> alloc_map_;
    FrameID range_begin_;
    FrameID range_end_;

    bool GetBit(FrameID frame) const;
    void SetBit(FrameID frame, bool allocated);
};

extern BitmapMemoryManager* memory_manager;
void InitializeMemoryManager(const MemoryMap& memory_map);

