#include <cstddef>
#include <vector>

namespace ets
{
/**
 * A circular buffer class backed by a std::vector. 
 * Stores up to maximum `n` items in the buffer
 * We can insert an item or get the oldest item in the buffer. Inserting an item might
 * override the oldest item in the buffer
 */
template<typename T>
class CircularBuffer
{
public:
  explicit CircularBuffer(std::size_t n) { _buffer.resize(n); }

  /**
   * Inserts a new item in the buffer
   * @param timestamp
   */
  void insert(T const& item) noexcept
  {
    _buffer[_index] = item;

    _index += 1;

    if (_index == _buffer.size())
    {
      _index = 0;
      _full = true;
    }
  }

  /**
   * Returns the oldest item in the buffer
   * @return
   */
  [[nodiscard]] T const& back() const noexcept
  {
    // _index here points to the next item to be replaced which is in the full buffer case it
    // is the oldest object
    return _full ? _buffer[_index] : _buffer[0];
  }

  [[nodiscard]] bool is_full() const noexcept { return _full; }

private:
  std::vector<T> _buffer;
  std::size_t _index{0};
  bool _full { false };
};
} // namespace ets