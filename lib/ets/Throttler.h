#include <vector>
#include <memory>
#include <chrono>
#include <type_traits>

#include "SlidingWindow.h"

namespace ets
{
/**
 * This is a message throttler class that also stores and queues messages.
 *
 * Usually each incoming message can be of a different type. Therefore, this class is templated to
 * support multiple type of messages.
 *
 * We pass the high priority message type as template parameter in order to prioritise that type
 * of messages over others.
 *
 * The rest messages are type erased and stored in vector and we access them later via a
 * virtual function
 */
template <typename THighPriorityMessage, typename TOnSendCallback>
class Throttler
{
public:
  Throttler(std::size_t max_messages, std::chrono::nanoseconds interval, TOnSendCallback on_send_callback)
  : sw(max_messages, interval), _on_send_callback(on_send_callback)
  {
  }

  /**
   * Tries to send a new message. If the message is throttled then returns the delay until the
   * end of the sliding window
   * @tparam TMessage
   * @param message
   * @return 0 if the message was sent, otherwise the delay until the next message can be send
   */
  template <typename TMessage>
  [[nodiscard]] std::chrono::nanoseconds try_send_message(TMessage const& message)
  {
    // first attempt to send the message
    std::chrono::nanoseconds const delay = sw.request();
    if (delay.count() == 0)
    {
      // we can send the message right now
      _on_send_callback.on_send(message);
      return std::chrono::nanoseconds{0};
    }

    // we throttled, but we know we can send a new message in next_message_ms milliseconds

    // store the message
    if constexpr (std::is_same_v<TMessage, THighPriorityMessage>)
    {
      // this is a high priority message and we store it as a high priority
      _high_priority_messages.push_back(message);
    }
    else
    {
      // to support and store any other message type we type erase it
      using stored_message_t = StoredMessage<TMessage>;
      _rest_messages.push_back(std::make_unique<stored_message_t>(_on_send_callback, message));
    }

    // our thread needs to look our queue in next_message_ms
    return delay;
  }

  /**
   * Send any messages in the queue
   * @return a zero delay means we have send all the messages, an non zero delay means we still
   * need to schedule to send more messages later
   */
  [[nodiscard]] std::chrono::nanoseconds send_queued_messages()
  {
    // first check and send any high priority messages
    std::chrono::nanoseconds delay = _send_queued_messages(_high_priority_messages);

    if (delay.count() == 0)
    {
      // we sent all high priority messages so continue to send the next messages
      delay = _send_queued_messages(_rest_messages);
    }

    return delay;
  }

private:
  template <typename TMessageContainer>
  [[nodiscard]] std::chrono::nanoseconds _send_queued_messages(TMessageContainer& message_container)
  {
    for (auto message_it = std::begin(message_container); message_it != std::end(message_container);)
    {
      std::chrono::nanoseconds const delay = sw.request();

      if (delay.count() != 0)
      {
        // message throttled return the delay until we can send the next message
        return delay;
      }

      if constexpr (std::is_same_v<THighPriorityMessage, typename TMessageContainer::value_type>)
      {
        _on_send_callback.on_send(*message_it);
      }
      else
      {
        // we need to send the message via the virtual function
        (*message_it)->send();
      }

      // also need to remove the message from the front of our vector
      message_it = message_container.erase(message_it);
    }

    // we send all messages in this container, we can just return 0
    return std::chrono::nanoseconds{0};
  }

private:
  class StoredMessageBase
  {
  public:
    explicit StoredMessageBase(TOnSendCallback& on_send_callback) : _on_send_callback(on_send_callback) {};
    virtual ~StoredMessageBase() = default;
    virtual void send() = 0;

  protected:
    TOnSendCallback& _on_send_callback;
  };

  template <typename TMessage>
  class StoredMessage : public StoredMessageBase
  {
  public:
    StoredMessage(TOnSendCallback& on_send_callback, TMessage const& message) :
    StoredMessageBase(on_send_callback), _message(message) {}

    void send() override { this->_on_send_callback.on_send(_message); }

  private:
    TMessage _message;
  };

protected:
  // protected to access for testing
  TOnSendCallback _on_send_callback;

private:
  SlidingWindow sw;

  // store highest priority messages in a separate vector to send them first.
  std::vector<THighPriorityMessage> _high_priority_messages;

  // any other message type is stored in the same vector and pushed at the back
  std::vector<std::unique_ptr<StoredMessageBase>> _rest_messages;
};
}