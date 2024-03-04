#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <deque>

#include <udpcap/udpcap_socket.h>

#include <ecaludp/error.h>

namespace ecaludp
{
  class AsyncUdpcapSocket
  {
  /////////////////////////////////////////////////////
  // Constructor/Destructor
  /////////////////////////////////////////////////////
  public:
    AsyncUdpcapSocket();
    ~AsyncUdpcapSocket();

    // Disable copy and move
    AsyncUdpcapSocket(const AsyncUdpcapSocket&)            = delete;
    AsyncUdpcapSocket(AsyncUdpcapSocket&&)                 = delete;
    AsyncUdpcapSocket& operator=(const AsyncUdpcapSocket&) = delete;
    AsyncUdpcapSocket& operator=(AsyncUdpcapSocket&&)      = delete;

  /////////////////////////////////////////////////////
  // udpcap forwarded methods
  /////////////////////////////////////////////////////
  public:
    bool isValid                    () const                                   { return udpcap_socket_.isValid(); }
    bool bind                       (const Udpcap::HostAddress& local_address, uint16_t local_port); // This also starts the wait thread for async receive
    bool isBound                    () const                                   { return udpcap_socket_.isBound(); }
    Udpcap::HostAddress localAddress() const                                   { return udpcap_socket_.localAddress(); }
    uint16_t localPort              () const                                   { return udpcap_socket_.localPort(); }
    bool setReceiveBufferSize       (int size)                                 { return udpcap_socket_.setReceiveBufferSize(size); }
    bool joinMulticastGroup         (const Udpcap::HostAddress& group_address) { return udpcap_socket_.joinMulticastGroup(group_address); }
    bool leaveMulticastGroup        (const Udpcap::HostAddress& group_address) { return udpcap_socket_.leaveMulticastGroup(group_address); }
    void setMulticastLoopbackEnabled(bool enabled)                             { udpcap_socket_.setMulticastLoopbackEnabled(enabled); }
    bool isMulticastLoopbackEnabled () const                                   { return udpcap_socket_.isMulticastLoopbackEnabled(); }
    void close                      ();

  /////////////////////////////////////////////////////
  // Public Methods
  /////////////////////////////////////////////////////
  public:
    void asyncReceiveFrom( char*                buffer
                         , size_t               max_buffer_size
                         , Udpcap::HostAddress& sender_address
                         , uint16_t&            sender_port
                         , const std::function<void(ecaludp::Error, size_t)>& read_handler);

  /////////////////////////////////////////////////////
  // Wait thread function
  /////////////////////////////////////////////////////
  private:
    void waitForData();

  /////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////
  private:
    struct AsyncReceiveFromParameters
    {
      char*                 buffer_;
      size_t                max_buffer_size_;
      Udpcap::HostAddress*  sender_address_;
      uint16_t*             sender_port_;
      std::function<void(ecaludp::Error, size_t)> read_handler_;
    };

    Udpcap::UdpcapSocket         udpcap_socket_;

    std::unique_ptr<std::thread> wait_thread_;
    std::mutex                   wait_thread_trigger_mutex_;
    std::condition_variable      wait_thread_trigger_cv_;
    std::deque<AsyncReceiveFromParameters> async_receive_from_parameters_queue_;
    bool                         is_closed;
  };
}
