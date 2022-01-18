#include "doctest.h"

#include "ets/CircularBuffer.h"
#include <cstdint>

TEST_SUITE_BEGIN("CircularBuffer");

using namespace ets;

/***/
TEST_CASE("insert and items")
{
  CircularBuffer<uint32_t> buffer {4};

  buffer.insert(1);
  REQUIRE_EQ(buffer.back(), 1);

  buffer.insert(2);
  REQUIRE_EQ(buffer.back(), 1);

  buffer.insert(3);
  REQUIRE_EQ(buffer.back(), 1);

  buffer.insert(4);
  REQUIRE_EQ(buffer.back(), 1);

  buffer.insert(5);
  REQUIRE_EQ(buffer.back(), 2);

  buffer.insert(6);
  REQUIRE_EQ(buffer.back(), 3);

  buffer.insert(7);
  REQUIRE_EQ(buffer.back(), 4);

  buffer.insert(8);
  REQUIRE_EQ(buffer.back(), 5);
}

/***/
TEST_CASE("insert and override items")
{
  constexpr uint32_t n = 4;
  CircularBuffer<uint32_t> buffer {n};

  for (uint32_t i = 0; i < 1000; ++i)
  {
    buffer.insert(i);

    if (i <= n - 1)
    {
      // buffer is not full yet
      REQUIRE_EQ(buffer.back(), 0);
    }
    else
    {
      REQUIRE_EQ(buffer.back(), i - n + 1);
    }
  }
}

/***/
TEST_CASE("check buffer is full")
{
  CircularBuffer<uint32_t> buffer {4};

  buffer.insert(1);
  REQUIRE_FALSE(buffer.is_full());

  buffer.insert(2);
  REQUIRE_FALSE(buffer.is_full());

  buffer.insert(3);
  REQUIRE_FALSE(buffer.is_full());

  buffer.insert(4);
  REQUIRE(buffer.is_full());

  buffer.insert(5);
  REQUIRE(buffer.is_full());

  buffer.insert(6);
  REQUIRE(buffer.is_full());
}

TEST_SUITE_END();