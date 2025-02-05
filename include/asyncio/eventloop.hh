#pragma once

#include <iostream>
#include <tuple>
#include <type_traits>

#include "asyncio/task.hh"

namespace asyncio {

class TaskBase;

class EventLoop {
  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;

 public:
  EventLoop() : size_{} {
    head_ = new TaskBase(nullptr);
    head_->set_next(head_);
    head_->set_prev(head_);
  }

  /// Release the memory for each task.
  ~EventLoop() {}

  void Run() {
    while (!IsEmpty()) {
      for (cur_ = head_->next(); cur_ != head_; cur_ = cur_->next()) {
        if (cur_->IsDone()) {
          std::cout << '[' << cur_->id() << "] is done!\n";

          RemoveTask(cur_);
          continue;
        }

        if (cur_->IsReady()) {
          std::cout << '[' << cur_->id() << "] is ready!\n";
          cur_->Resume();
        } else if (cur_->IsBlocked()) {
          // std::cout << '[' << cur_->id() << "] is blocked!\n";
        }
      }
    }
  }

  template <typename... Task>
  auto Gather(Task&&... tasks) -> std::tuple<typename Task::type...> {
    (Append(tasks), ...);
    Run();

    return std::make_tuple(tasks.Get()...);
  }

  TaskBase* GetCurrentTask() const { return cur_; }

 private:
  bool IsEmpty() const { return size_ == 0; }

  void RemoveTask(TaskBase* node) {
    std::cout << "Remove task: " << node << std::endl;

    auto prev = node->prev();
    auto next = node->next();

    prev->set_next(next);
    next->set_prev(prev);

    size_--;
  }

  template <typename Task>
  void Append(Task&& task) {
    using T = std::decay_t<Task>;

    TaskBase* node = new T(task);
    auto tail = head_->prev();
    tail->set_next(node);
    node->set_prev(tail);
    node->set_next(head_);
    head_->set_prev(node);

    size_++;
  }

  TaskBase* head_;
  TaskBase* cur_;
  std::size_t size_;
};

EventLoop& get_event_loop();
TaskBase* current_task();

}  // namespace asyncio