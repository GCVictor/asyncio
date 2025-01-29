#pragma once

#include <memory>

namespace asyncio {

class TaskBase;

class EventLoop {
 public:
  void Run();
  void Gather();

 private:
  std::unique_ptr<TaskBase> head_;
};

}  // namespace asyncio