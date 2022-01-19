#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <variant>

#include "ets/Throttler.h"

/**
 * Odysseas Georgoudis
 * Exercise 4: Restaurant Throughput Management (C++17)
 */

/**
 * Example program using Throttler class
 */

/**
 * Different types of order messages
 */
struct NewOrder
{
  explicit NewOrder(std::string desc) : desc(std::move(desc)){};
  std::string desc;
};

struct AmendOrder
{
  explicit AmendOrder(std::string desc) : desc(std::move(desc)){};
  std::string desc;
};

struct CancelOrder
{
  explicit CancelOrder(std::string desc) : desc(std::move(desc)){};
  std::string desc;
};

/**
 * A class providing a callback of how to send the orders. This is used by the throttle class
 */
struct OnSendCallback
{
  template <typename TMessage>
  void on_send(TMessage const& message)
  {
    // Here we just print the message instead of sending it
    std::cout << "Sending message: " << message.desc << std::endl;
  }
};

/**
 * A very simple inefficient queue for the threads to communicate, just to demonstrate the example
 */
template <typename T>
class ThreadSafeQueue
{
public:
  void push(T&& item)
  {
    std::lock_guard lock{_mutex};
    _queue.push(std::move(item));
  }

  T const* front()
  {
    std::lock_guard lock{_mutex};
    return !_queue.empty() ? std::addressof(_queue.front()) : nullptr;
  }

  void pop()
  {
    std::lock_guard lock{_mutex};
    _queue.pop();
  }

private:
  std::mutex _mutex;
  std::queue<T> _queue;
};

using message_queue_types_t =
  std::variant<std::unique_ptr<NewOrder>, std::unique_ptr<AmendOrder>, std::unique_ptr<CancelOrder>>;
using message_queue_t = ThreadSafeQueue<message_queue_types_t>;

/**
 * This is the meal processor thread. This is the user of the throttle class
 */
class MealProcessor
{
public:
  explicit MealProcessor(message_queue_t& message_queue) : _message_queue(message_queue){};

  ~MealProcessor()
  {
    if (_worker.joinable())
    {
      _worker.join();
    }
  }

  void run()
  {
    std::thread worker{[this]() { _main_loop(); }};
    _worker.swap(worker);
  }

private:
  void _main_loop()
  {
    // This is a never ending loop so the program has to be killed manually.
    while (true)
    {
      // read any messages from our queue and attempt to send them
      _process_message_queue();

      // check if we have any queued messages to send
      _send_queued_orders();
    }
  }

  /**
   * Process all incoming messages from clients. The messages will go through the Throttle class
   * and will either get be sent or get queued for later
   */
  void _process_message_queue()
  {
    // Get the first message from the queue
    message_queue_types_t const* next_message = _message_queue.front();

    while (next_message)
    {
      // type-matching visitor
      std::visit(
        [this](auto&& arg)
        {
          using TMessageType = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<TMessageType, std::unique_ptr<NewOrder>> ||
                        std::is_same_v<TMessageType, std::unique_ptr<AmendOrder>> ||
                        std::is_same_v<TMessageType, std::unique_ptr<CancelOrder>>)
          {
            // Check if we can send this message via the throttler
            std::chrono::nanoseconds const delay = _throttler.template try_send_message(*arg);

            if (delay.count() != 0)
            {
              // if the returned delay is not zero it means some messages got throttled
              _scheduled = std::chrono::steady_clock::now() + delay;
            }
            else
            {
              // else set _scheduled to empty to indicate there is nothing else
              _scheduled = std::chrono::steady_clock::time_point{};
            }
          }
        },
        *next_message);

      // remove this message and read the next
      _message_queue.pop();
      next_message = _message_queue.front();
    }
  };

  /**
   * Send any previously queued orders
   */
  void _send_queued_orders()
  {
    if ((_scheduled != std::chrono::steady_clock::time_point{}) && std::chrono::steady_clock::now() >= _scheduled)
    {
      // if we are past the point of sending orders send any queued orders
      std::chrono::nanoseconds const delay = _throttler.send_queued_messages();

      if (delay.count() != 0)
      {
        // if the delay is not zero it means some messages are still queued
        _scheduled = std::chrono::steady_clock::now() + delay;
      }
      else
      {
        // else set _scheduled to empty to indicate there is nothing else
        _scheduled = std::chrono::steady_clock::time_point{};
      }
    }
  }

private:
  message_queue_t& _message_queue;
  std::thread _worker;
  ets::Throttler<CancelOrder, OnSendCallback> _throttler{3, std::chrono::seconds{1}, OnSendCallback{}};
  std::chrono::steady_clock::time_point _scheduled{};
};

/**
 * A client thread producing order messages
 */
class Client
{
public:
  Client(message_queue_t& message_queue, uint32_t client_id)
    : _message_queue(message_queue), _client_id(client_id){};

  ~Client()
  {
    if (_worker.joinable())
    {
      _worker.join();
    }
  }

  void run()
  {
    std::thread worker{[this]() { _produce_orders(); }};
    _worker.swap(worker);
  }

private:
  void _produce_orders()
  {
    // send 3 orders
    push_new_order();
    push_amend_order();
    push_cancel_order();

    // send 4 amends
    push_amend_order();
    push_amend_order();
    push_amend_order();
    push_amend_order();

    // send 4 cancels
    push_cancel_order();
    push_cancel_order();
    push_cancel_order();
    push_cancel_order();
  }

  void push_new_order()
  {
    auto new_order = std::make_unique<NewOrder>("New Order Id: " + std::to_string(_order_id) +
                                                " from client " + std::to_string(_client_id));
    _message_queue.push(std::move(new_order));
    _order_id += 1;
  }

  void push_amend_order()
  {
    auto amend_order = std::make_unique<AmendOrder>("Amend Order Id: " + std::to_string(_order_id) +
                                                    " from client " + std::to_string(_client_id));
    _message_queue.push(std::move(amend_order));
    _order_id += 1;
  }

  void push_cancel_order()
  {
    auto cancel_order = std::make_unique<CancelOrder>("Cancel Order Id: " + std::to_string(_order_id) +
                                                      " from client " + std::to_string(_client_id));
    _message_queue.push(std::move(cancel_order));
    _order_id += 1;
  }

private:
  message_queue_t& _message_queue;
  uint32_t _client_id;
  uint32_t _order_id{0};
  std::thread _worker;
};

int main()
{
  message_queue_t message_queue;

  MealProcessor mp{message_queue};
  mp.run();

  Client client_1{message_queue, 1};
  client_1.run();

  //  Client client_2{message_queue, 2};
  //  client_2.run();

  return 0;
}
