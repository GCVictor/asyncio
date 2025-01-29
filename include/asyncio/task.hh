#pragma once

#include <coroutine>
#include <stdexcept>

namespace asyncio {

class EventLoop;
extern thread_local EventLoop loop;

class TaskBase {
 public:
  enum State {
    kReady,
    kBlocked,
  };

  /// Constructs a TaskBase object with a coroutine handle and an optional next
  /// task.
  ///
  /// \param handle The coroutine handle associated with the task.
  TaskBase(std::coroutine_handle<> handle);

  /// \return The coroutine handle.
  auto handle() const { return handle_; }

  /// \return The next task which may be nullptr if there is no next task.
  TaskBase* next() const { return next_; }

  /// Resumes the coroutine associated with this task.
  /// If the coroutine is already finished, this function does nothing.
  void Resume() {
    if (IsDone()) {
      return;
    }

    handle_.resume();
  }

  void SetReady() { state_ = State::kReady; }
  void SetBlocked() { state_ = State::kBlocked; }
  constexpr bool IsReady() const { return state_ == State::kReady; }
  constexpr bool IsBlocked() const { return state_ == State::kBlocked; }
  bool IsDone() const { return handle_.done(); }

 protected:
  EventLoop& owner_;                ///< The event loop that owns this task.
  std::coroutine_handle<> handle_;  ///< The coroutine handle for this task.
  TaskBase* next_;                  ///< Pointer to next task.
  State state_;
};

template <typename T>
struct Task : public TaskBase {
  struct promise_type;

  explicit Task(std::coroutine_handle<promise_type> handle)
      : TaskBase{handle} {}

  struct promise_type {
    T value;
    std::exception_ptr exception;

    auto get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    auto initial_suspend() { return std::suspend_never{}; }

    auto final_suspend() noexcept { return std::suspend_always{}; }

    void return_value(const T& val) { value = val; }

    void unhandled_exception() { exception = std::current_exception(); }

    ~promise_type() {
      if (exception) {
        std::rethrow_exception(exception);
      }
    }
  };

  T Get() const {
    if (!IsDone()) {
      throw std::runtime_error("Coroutine is not done or invalid handle.");
    }

    auto handle =
        std::coroutine_handle<promise_type>::from_address(handle_.address());
    auto promise = handle.promise();

    if (promise.exception) {
      std::rethrow_exception(promise.exception);
    }

    return std::move(promise.value);
  }
};
}  // namespace asyncio
