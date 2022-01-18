#include "doctest.h"

#include <cstdint>
#include <chrono>
#include <thread>

#include "ets/SlidingWindow.h"

TEST_SUITE_BEGIN("SlidingWindow");

using namespace ets;

/***/
TEST_CASE("request and check")
{
  // Accept 100 requests per second
  SlidingWindow sw { 100, std::chrono::seconds {1} };

  // First sleep for 500 ms
  std::this_thread::sleep_for(std::chrono::milliseconds{500});

  // Then send 90 requests
  for (uint32_t i = 0; i < 90; ++i)
  {
    // All the requests should pass returning 0 delay
    auto const delay = sw.request().count();
    REQUIRE_EQ(delay, 0);
  }

  // Now sleep 600 ms
  std::this_thread::sleep_for(std::chrono::milliseconds{600});

  // Then send 10 more requests
  for (uint32_t i = 0; i < 10; ++i)
  {
    // All the requests should pass returning 0 delay
    auto const delay = sw.request().count();
    REQUIRE_EQ(delay, 0);
  }

  // We have now reached the maximum messages and any other request should fail for the next 400 ms
  // here we assume there is no slowdown in the system for this test to pass ..
  for (uint32_t i = 0; i < 10; ++i)
  {
    // All the requests should fail returning a delay
    auto const delay = sw.request().count();
    REQUIRE_GT(delay, 0);
  }
}

TEST_SUITE_END();