#pragma once

#include <atomic>
#include <chrono>
#include <coroutine>
#include <mutex>
#include <stdexcept>
#include <thread>

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

    void await_suspend(std::coroutine_handle<> h) {
      std::unique_lock lock(timer_.mutex_);

      if (timer_.state_ != State::Pending) {
        return;
      }

      timer_.coroutine_ = h;
      timer_.thread_ = std::thread([this, delay = timer_.delay_] {
        std::this_thread::sleep_for(delay);
        std::unique_lock lock(timer_.mutex_);

        if (timer_.state_ == State::Pending) {
          timer_.state_ = State::Completed;

          if (timer_.coroutine_) {
            timer_.coroutine_.resume();
          }
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

      if (thread_.joinable()) {
        thread_.join();
      }

      if (coroutine_) {
        coroutine_.destroy();
      }
    }
  }

 private:
  std::chrono::steady_clock::duration delay_;
  std::thread thread_;
  std::coroutine_handle<> coroutine_;
  std::mutex mutex_;
  std::atomic<State> state_;
};

}