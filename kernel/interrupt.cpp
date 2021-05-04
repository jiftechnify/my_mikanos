#include "interrupt.hpp"

// 割り込み記述子テーブル
std::array<InterruptDescriptor, 256> idt;

// 割り込み記述子に値をセットする
void SetIDTEntry(InterruptDescriptor& desc, InterruptDescriptorAttribute attr, uint64_t offset, uint16_t segment_selector) {
  desc.attr = attr;
  desc.offset_low = offset & 0xffffu;
  desc.offset_middle = (offset >> 16) & 0xffffu;
  desc.offset_high = offset >> 32;
  desc.segment_selector = segment_selector;
}

// 割り込みハンドラの処理が終了したことをCPUに通知する
// アドレス 0xfee000b0 は End of Interrupt レジスタといい、値を書き込むと通知したことになる
void NotifyEndOfInterrupt() {
  // volatile はコンパイラによる最適化されて書き込み処理が消えるのを防ぐ
  volatile auto end_of_interrupt = reinterpret_cast<uint32_t*>(0xfee000b0);
  *end_of_interrupt = 0;
}

