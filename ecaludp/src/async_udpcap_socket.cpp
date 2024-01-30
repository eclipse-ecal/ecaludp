#include "async_udpcap_socket.h"

#include <iostream> // TODO: Remove

namespace ecaludp
{
  AsyncUdpcapSocket::AsyncUdpcapSocket()
    : udpcap_socket_()
    , is_closed(false)
  {}

  AsyncUdpcapSocket::~AsyncUdpcapSocket()
  {
    close();
    // TODO: Stop the socket and the wait thread
    if (wait_thread_)
    {
      wait_thread_->join();
    }

    // TODO: implement
  }

  void AsyncUdpcapSocket::asyncReceiveFrom( char*                buffer
                                          , size_t               max_buffer_size
                                          , Udpcap::HostAddress& sender_address
                                          , uint16_t&            sender_port
                                          , const std::function<void(ecaludp::Error, size_t)>& read_handler)
  {
    std::unique_lock<std::mutex> lock(wait_thread_trigger_mutex_);
    std::cerr << "===Pushing async receive from parameters" << std::endl; // TODO REMOVE
    async_receive_from_parameters_queue_.push_back({ buffer, max_buffer_size, &sender_address, &sender_port, read_handler });
    wait_thread_trigger_cv_.notify_one();
  }
  /////////////////////////////////////////////////////
  // udpcap forwarded methods
  /////////////////////////////////////////////////////
  bool AsyncUdpcapSocket::bind(const Udpcap::HostAddress& local_address, uint16_t local_port)
  {
    bool success = udpcap_socket_.bind(local_address, local_port);

    if (success)
    {
      if (wait_thread_ && wait_thread_->joinable())
      {
        wait_thread_->join();
      }

      is_closed = false;

      wait_thread_ = std::make_unique<std::thread>(&AsyncUdpcapSocket::waitForData, this);
    }

    return success;
  }

  void AsyncUdpcapSocket::close()
  {
    udpcap_socket_.close();
    {
      std::lock_guard<std::mutex> lock(wait_thread_trigger_mutex_);
      is_closed = true;
      wait_thread_trigger_cv_.notify_one();
    }
  }

  void AsyncUdpcapSocket::waitForData()
  {
    while (true)
    {
      //TODO: Revise this function, it is very important, but currently not safe.

      AsyncReceiveFromParameters next_async_receive_from_parameters{};

      // Wait until there is somebody requesting some data. This is done by waiting for the callback queue to be non-empty.
      {
        std::unique_lock<std::mutex> lock(wait_thread_trigger_mutex_);
        wait_thread_trigger_cv_.wait(lock, [this] { return is_closed || !async_receive_from_parameters_queue_.empty(); });

        if (!async_receive_from_parameters_queue_.empty())
        {
          next_async_receive_from_parameters = async_receive_from_parameters_queue_.front();
          async_receive_from_parameters_queue_.pop_front();
        }

        if (async_receive_from_parameters_queue_.empty() && is_closed)
        {
          return;
        }
      }
    
      if (udpcap_socket_.isBound())
      {
        std::cerr << "===Start receiving data" << std::endl; // TODO REMOVE
        size_t rec_bytes = udpcap_socket_.receiveDatagram(next_async_receive_from_parameters.buffer_
                                                        , next_async_receive_from_parameters.max_buffer_size_
                                                        , next_async_receive_from_parameters.sender_address_
                                                        , next_async_receive_from_parameters.sender_port_);
          
        std::cerr << "===Received " << rec_bytes << " bytes from " << next_async_receive_from_parameters.sender_address_->toString() << ":" << std::to_string(*next_async_receive_from_parameters.sender_port_) << std::endl; // TODO REMOVE

        if (next_async_receive_from_parameters.sender_address_->isValid())
        {
          next_async_receive_from_parameters.read_handler_(ecaludp::Error::OK, rec_bytes);
        }
        else
        {
          next_async_receive_from_parameters.read_handler_(ecaludp::Error::GENERIC_ERROR, rec_bytes);
        }
      }
      else
      {
        next_async_receive_from_parameters.read_handler_(ecaludp::Error::GENERIC_ERROR, 0);
      }
    }
  }
}
