#pragma once

#include <cstdint> 
#include <queue>
#include <vector>
#include "message.hpp"

void StartLAPICTimer();
uint32_t LAPICTimerElapsed();
void StopLAPICTimer();

class Timer {
  public:
    Timer(unsigned long timeout, int value);
    unsigned long Timeout() const { return timeout_; }
    int Value() const { return value_; }

  private:
    unsigned long timeout_;
    int value_;
};

// タイムアウトが遠いTimerほど順序的に小さいとみなす
// この順序を優先度と考えると、「タイムアウトが遠いほど優先度が低い」ことを意味する
inline bool operator<(const Timer& lhs, const Timer& rhs) {
  return lhs.Timeout() > rhs.Timeout();
}

class TimerManager {
  public:
    TimerManager(std::deque<Message>& msg_queue);
    void AddTimer(const Timer& timer);
    void Tick();
    unsigned long CurrentTick() const { return tick_; }

  private:
    volatile unsigned long tick_{0};
    std::priority_queue<Timer> timers_{};
    std::deque<Message>& msg_queue_;
};

extern TimerManager* timer_manager;
extern unsigned long lapic_timer_freq; // 1秒あたりのLocal APIC タイマのカウント数
const int kTimerFreq = 100;

void InitializeLAPICTimer(std::deque<Message>& msg_queue);
void LAPICTimerOnInterrupt();

