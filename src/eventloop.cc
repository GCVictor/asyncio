#include "asyncio/eventloop.h"

#include "asyncio/task.hh"

namespace asyncio {

thread_local EventLoop loop;

void EventLoop::Run() {}

}  // namespace asyncio