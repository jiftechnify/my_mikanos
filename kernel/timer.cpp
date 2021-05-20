#include "timer.hpp"
#include "interrupt.hpp"
#include "acpi.hpp"
#include "task.hpp"

namespace {
  const uint32_t kCountMax = 0xffffffffu;
  volatile uint32_t& lvt_timer = *reinterpret_cast<uint32_t*>(0xfee00320);
  volatile uint32_t& initial_count = *reinterpret_cast<uint32_t*>(0xfee00380);
  volatile uint32_t& current_count = *reinterpret_cast<uint32_t*>(0xfee00390);
  volatile uint32_t& divide_config = *reinterpret_cast<uint32_t*>(0xfee003e0);
}

void StartLAPICTimer() {
  initial_count = kCountMax;
}

uint32_t LAPICTimerElapsed() {
  return kCountMax - current_count;
}

void StopLAPICTimer() {
  initial_count = 0;
}

/**
 * Timer
 */
Timer::Timer(unsigned long timeout, int value, uint64_t task_id)
    : timeout_{timeout}, value_{value}, task_id_{task_id} {
}

/**
 * TimerManager
 */
TimerManager::TimerManager() {
  timers_.push(Timer{std::numeric_limits<unsigned long>::max(), 0, 0});
}

void TimerManager::AddTimer(const Timer& timer) {
  timers_.push(timer);
}

bool TimerManager::Tick() {
  ++tick_;
  // timeoutを迎えたTimerがあれば、それを生成したタスクにメッセージを送信
  bool task_timer_timeout = false;
  while (true) {
    const auto& t = timers_.top();
    if (t.Timeout() > tick_) {
      break;
    }

    if (t.Value() == kTaskTimerValue) {
      // タスク切り替えタイマがタイムアウトした場合
      task_timer_timeout = true;
      timers_.pop();
      timers_.push(Timer{tick_ + kTaskTimerPeriod, kTaskTimerValue, 1});
      continue;
    }

    Message m{Message::kTimerTimeout};
    m.arg.timer.timeout = t.Timeout();
    m.arg.timer.value = t.Value();
    task_manager->SendMessage(t.TaskID(), m);

    timers_.pop();
  }

  return task_timer_timeout;
}

TimerManager* timer_manager;
unsigned long lapic_timer_freq;

// Local APIC タイマーの周期ごとに割り込みハンドラから呼び出される処理
extern "C" void LAPICTimerOnInterrupt(const TaskContext& ctx_stack) {
  const bool task_timer_timeout = timer_manager->Tick();
  NotifyEndOfInterrupt(); // 先にタスクを切り替えてしまうと、これが呼び出されなくなってしまう

  if (task_timer_timeout) {
    task_manager->SwitchTask(ctx_stack);
  }
}

// Local APIC タイマー初期化
void InitializeLAPICTimer() {
  timer_manager = new TimerManager();

  divide_config = 0b1011; // divide 1:1
  lvt_timer = 0b001 << 1;  // masked, one-shot
  
  StartLAPICTimer();
  acpi::WaitMilliseconds(100);
  const auto elapsed = LAPICTimerElapsed();
  StopLAPICTimer();

  lapic_timer_freq = static_cast<unsigned long>(elapsed) * 10;

  divide_config = 0b1011; // divide 1:1
  lvt_timer = (0b010 << 16) | InterruptVector::kLAPICTimer;  // not-masked, periodic
  initial_count = lapic_timer_freq / kTimerFreq;
}

