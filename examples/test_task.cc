#include <gtest/gtest.h>

#include "asyncio/task.hh"

asyncio::Task<int> return_int() { co_return 42; }

TEST(Task, SingleReturn) {
  auto task = return_int();

  int value = task.Get();

  EXPECT_EQ(42, value);
}
