#include "asyncio/eventloop.hh"

namespace asyncio {

thread_local EventLoop loop;

EventLoop& get_event_loop() { return loop; }

TaskBase* current_task() { return loop.GetCurrentTask(); }

}  // namespace asyncio