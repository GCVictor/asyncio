#include <gtest/gtest.h>

#include "asyncio/eventloop.hh"
#include "asyncio/task.hh"
#include "asyncio/timer.hh"

asyncio::Task<int> return_int() { co_return 42; }

asyncio::Task<int> sleep_ms(int n) {
  co_await asyncio::Timer{std::chrono::seconds{n}};
  co_await asyncio::Timer{std::chrono::seconds{n}};
  co_return 1;
}

TEST(EventLoop, Gather) {
  auto& loop = asyncio::get_event_loop();

  loop.Gather(return_int(), return_int());
  loop.Run();
}

TEST(EventLoop, Run) {
  int n = 2;
  auto& loop = asyncio::get_event_loop();
  loop.Gather(sleep_ms(n), sleep_ms(n), sleep_ms(n));
  auto start = std::chrono::steady_clock::now();
  loop.Run();
  auto end = std::chrono::steady_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  std::cout << "Elapsed time: " << elapsed_ms / 1000.0 << " secs" << std::endl;
}