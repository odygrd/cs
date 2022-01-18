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
    _buffer[_head % _buffer.size()] = item;
    _head += 1;
  }

  /**
   * Returns the oldest item in the buffer
   * @return
   */
  [[nodiscard]] T const& back() const noexcept
  {
    return (_head < _buffer.size()) ? _buffer[0] : _buffer[_head % _buffer.size()];
  }

  [[nodiscard]] bool is_full() const noexcept { return _head >= _buffer.size(); }

private:
  std::vector<T> _buffer;
  std::size_t _head{0};
};
} // namespace ets