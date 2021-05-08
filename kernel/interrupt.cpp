#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "timer.hpp"

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

namespace {
  std::deque<Message>* msg_queue;

  // xHCI用割り込みハンドラ
  __attribute__((interrupt))
  void IntHandlerXHCI(InterruptFrame* frame) {
    msg_queue->push_back(Message{Message::kInterruptXHCI});
    NotifyEndOfInterrupt();
  }

  __attribute__((interrupt))
  void IntHandlerLAPICTimer(InterruptFrame* frame) {
    LAPICTimerOnInterrupt();
    NotifyEndOfInterrupt();
  }
}

void InitializeInterrupt(std::deque<Message>* msg_queue) {
  ::msg_queue = msg_queue;

  SetIDTEntry(idt[InterruptVector::kXHCI], 
              MakeIDTAttr(DescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerXHCI),
              kKernelCS);
  SetIDTEntry(idt[InterruptVector::kLAPICTimer],
              MakeIDTAttr(DescriptorType::kInterruptGate, 0),
              reinterpret_cast<uint64_t>(IntHandlerLAPICTimer),
              kKernelCS);
  LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));
}

