#pragma once

#include <atomic>
#include <chrono>
#include <coroutine>
#include <mutex>
#include <stdexcept>
#include <thread>

#include "asyncio/eventloop.hh"
#include "asyncio/task.hh"

namespace asyncio {

class Timer {
 public:
  enum class State { Pending, Completed, Cancelled };

  explicit Timer(std::chrono::steady_clock::duration delay)
      : delay_(delay), state_(State::Pending) {}

  ~Timer() { Cancel(); }

  struct Awaiter {
    explicit Awaiter(Timer& timer) : timer_(timer) {}

    bool await_ready() const noexcept {
      return timer_.state_ == State::Completed;
    }

    void await_suspend(std::coroutine_handle<>) {
      std::unique_lock lock(timer_.mutex_);

      if (timer_.state_ != State::Pending) {
        return;
      }

      auto task = current_task();
      task->SetBlocked();

      timer_.thread_ = std::thread([this, task, delay = timer_.delay_] {
        std::this_thread::sleep_for(delay);
        std::unique_lock lock(timer_.mutex_);

        if (timer_.state_ == State::Pending) {
          timer_.state_ = State::Completed;
          task->SetReady();
        }
      });
    }

    void await_resume() {
      if (timer_.state_ == State::Cancelled) {
        throw std::runtime_error("Timer cancelled");
      }
    }

    Timer& timer_;
  };

  auto operator co_await() { return Awaiter{*this}; }

  void Cancel() {
    std::unique_lock lock(mutex_);

    if (state_ == State::Pending) {
      state_ = State::Cancelled;
    }

    if (thread_.joinable()) {
      thread_.join();
    }
  }

 private:
  std::chrono::steady_clock::duration delay_;
  std::thread thread_;
  std::mutex mutex_;
  std::atomic<State> state_;
};

}  // namespace asyncio