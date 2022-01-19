#include "doctest.h"

#include <chrono>
#include <cstdint>
#include <thread>

#include "ets/Throttler.h"

TEST_SUITE_BEGIN("Throttler");

using namespace ets;

struct HighPrioMsg
{
};

struct LowPrioMsg
{
};

struct OnSendCallback
{
  void on_send(HighPrioMsg const&)
  {
    ++high_prior_counter;
  }

  void on_send(LowPrioMsg const&)
  {
    ++low_prior_counter;
  }

  size_t high_prior_counter{0};
  size_t low_prior_counter{0};
};

class MockThrottler : public Throttler<HighPrioMsg,OnSendCallback>
  {
  public:
    using base_t = Throttler<HighPrioMsg,OnSendCallback>;
    using base_t::base_t;

    OnSendCallback const& get_on_send() { return _on_send_callback; }
  };

/***/
TEST_CASE("send message throttle and queue")
{
  // Accept 100 requests per second
  MockThrottler throttler {100, std::chrono::seconds{1}, OnSendCallback {}};

  // First sleep for 500 ms
  std::this_thread::sleep_for(std::chrono::milliseconds{500});

  // Then send 90 requests
  for (uint32_t i = 0; i < 90; ++i)
  {
    // All the requests should pass returning 0 delay
    auto const delay = throttler.try_send_message(LowPrioMsg{}).count();
    REQUIRE_EQ(delay, 0);
  }

  // check we call on_send
  REQUIRE_EQ(throttler.get_on_send().high_prior_counter, 0);
  REQUIRE_EQ(throttler.get_on_send().low_prior_counter, 90);

  // Now sleep 600 ms
  std::this_thread::sleep_for(std::chrono::milliseconds{600});

  // Then send 10 more requests
  for (uint32_t i = 0; i < 10; ++i)
  {
    // All the requests should pass returning 0 delay
    auto const delay = throttler.try_send_message(HighPrioMsg{}).count();
    REQUIRE_EQ(delay, 0);
  }

  // check we call on_send
  REQUIRE_EQ(throttler.get_on_send().high_prior_counter, 10);
  REQUIRE_EQ(throttler.get_on_send().low_prior_counter, 90);

  // We have now reached the maximum messages and any other request should fail for the next 400 ms
  for (uint32_t i = 0; i < 10; ++i)
  {
    // All the messages should fail returning a delay
    auto delay = throttler.try_send_message(HighPrioMsg{}).count();
    REQUIRE_GT(delay, 0);
    delay = throttler.try_send_message(LowPrioMsg{}).count();
    REQUIRE_GT(delay, 0);
  }

  // Now block and wait until we sent everything in the queue
  auto delay = throttler.send_queued_messages().count();
  while(delay != 0)
  {
    delay = throttler.send_queued_messages().count();

    if (throttler.get_on_send().low_prior_counter > 90)
    {
      // this means we started sending low priority messages.
      // check that first we sent all high priority messages
      REQUIRE_EQ(throttler.get_on_send().high_prior_counter, 20);
    }
  }

  // Check we only send all the messages
  REQUIRE_EQ(throttler.get_on_send().high_prior_counter, 20);
  REQUIRE_EQ(throttler.get_on_send().low_prior_counter, 100);
}

TEST_SUITE_END();