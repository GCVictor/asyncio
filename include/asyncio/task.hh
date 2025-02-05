#pragma once

#include <coroutine>
#include <stdexcept>

namespace asyncio {

class EventLoop;
extern thread_local EventLoop loop;

struct DummyVoid {};

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
  constexpr auto handle() const { return handle_; }

  /// \return The next task which may be nullptr if there is no next task.
  TaskBase* next() const { return next_; }
  void set_next(TaskBase* next) { next_ = next; }

  /// \return The previous task which may be nullptr if there is no previous
  ///         task.
  TaskBase* prev() const { return prev_; }
  void set_prev(TaskBase* prev) { prev_ = prev; }

  constexpr int id() const { return id_; }

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
  TaskBase* prev_;                  ///< Pointer to previous task.
  TaskBase* next_;                  ///< Pointer to next task.
  State state_;                     ///< The task state.
  int id_;
  static int unique_id_;
};

template <typename T = DummyVoid, bool = std::is_same_v<T, DummyVoid>>
struct Task : public TaskBase {
  using type = T;

  explicit Task(std::coroutine_handle<> handle) : TaskBase{handle} {}

  struct promise_type {
    std::exception_ptr exception;

    auto get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    void return_void() {}
    void unhandled_exception() { exception = std::current_exception(); }

    ~promise_type() {
      if (exception) {
        std::rethrow_exception(exception);
      }
    }
  };

  auto Get() {
    static struct DummyVoid instance;

    return instance;
  }
};

template <typename T>
struct Task<T, false> : public TaskBase {
  using type = T;

  struct promise_type;

  explicit Task(std::coroutine_handle<promise_type> handle)
      : TaskBase{handle} {}

  struct promise_type {
    T value;
    std::exception_ptr exception;

    auto get_return_object() {
      return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    auto initial_suspend() { return std::suspend_always{}; }
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
