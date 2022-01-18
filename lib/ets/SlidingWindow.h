#include <chrono>
#include <cstddef>
#include "CircularBuffer.h"

namespace ets
{
/**
 * Tracks send messages in the configured sliding window
 * This class uses a circular buffer where it stores timestamps.
 */
class SlidingWindow
{
public:
  SlidingWindow(std::size_t max_messages, std::chrono::nanoseconds interval)
    : _max_messages(max_messages), _interval(interval), _buffer(_max_messages)
  {
  }

  /**
   * Request to send a new message
   * @return how many milliseconds left until we can send a message or 0 if the message was
   * sent without any delay
   */
  [[nodiscard]] std::chrono::nanoseconds request()
  {
    // The back message in the buffer is the oldest.
    // We compare the current time with the back message.
    // If it now falls outside the window we can send a new message.
    // If it doesn't and the buffer is full of messages, then we have hit the limit
    auto const now = std::chrono::steady_clock::now();
    auto const dif_from_oldest = std::chrono::duration_cast<std::chrono::nanoseconds>(now - _buffer.back());

    if ((dif_from_oldest <= _interval) && _buffer.is_full())
    {
      // we can not send more messages, return the milliseconds we can send a new message
      return _interval - dif_from_oldest;
    }

    _buffer.insert(now);

    return std::chrono::nanoseconds{0};
  }

private:
  std::size_t _max_messages;
  std::chrono::nanoseconds _interval;
  CircularBuffer<std::chrono::steady_clock::time_point> _buffer;
};
}