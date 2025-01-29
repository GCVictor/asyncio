#include <asyncio/task.hh>

namespace asyncio {

TaskBase::TaskBase(std::coroutine_handle<> handle)
    : owner_{loop}, handle_{handle}, next_{}, state_{State::kReady} {}

}  // namespace asyncio