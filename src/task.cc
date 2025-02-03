#include "asyncio/task.hh"

namespace asyncio {

int TaskBase::unique_id_ = 0;

TaskBase::TaskBase(std::coroutine_handle<> handle)
    : owner_{loop},
      handle_{handle},
      next_{},
      state_{State::kReady},
      id_{unique_id_++} {}

}  // namespace asyncio