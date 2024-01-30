#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>

#include <asio.hpp>

#include <ecaludp/error.h>
#include <ecaludp/owning_buffer.h>
#include <ecaludp/raw_memory.h>
#include <vector>

#include <ecaludp/ecaludp_export.h>

namespace ecaludp
{
  namespace v5
  {
    class Reassembly;
  }

  class recycle_shared_pool;
  class AsyncUdpcapSocket;

  class SocketUdpcap
  {
  /////////////////////////////////////////////////////////////////
  // Constructor
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT SocketUdpcap(std::array<char, 4> magic_header_bytes);

    // Destructor
    ECALUDP_EXPORT ~SocketUdpcap();

    // Disable copy constructor and assignment operator
    SocketUdpcap(const SocketUdpcap&)             = delete;
    SocketUdpcap& operator=(const SocketUdpcap&)  = delete;

    // Disable move constructor and assignment operator
    SocketUdpcap(SocketUdpcap&&)            = delete;
    SocketUdpcap& operator=(SocketUdpcap&&) = delete;

  /////////////////////////////////////////////////////////////////
  // Settings
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT void set_max_reassembly_age(std::chrono::steady_clock::duration max_reassembly_age);
    ECALUDP_EXPORT std::chrono::steady_clock::duration get_max_reassembly_age() const;

  /////////////////////////////////////////////////////////////////
  // API "Passthrough" (and a bit conversion to asio types)
  /////////////////////////////////////////////////////////////////
  public:
    bool is_valid() const;
    bool bind(const asio::ip::udp::endpoint& sender_endpoint);
    bool is_bound() const;
    asio::ip::udp::endpoint local_endpoint();
    bool set_receive_buffer_size(int size);
    bool has_pending_datagrams() const;
    bool join_multicast_group(const asio::ip::address_v4& group_address);
    bool leave_multicast_group(const asio::ip::address_v4& group_address);
    void set_multicast_loopback_enabled(bool enabled);
    bool is_multicast_loopback_enabled() const;
    void close();

  /////////////////////////////////////////////////////////////////
  // Receiving
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT void async_receive_from(asio::ip::udp::endpoint& sender_endpoint
                                  , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, ecaludp::Error)>& completion_handler);


  private:
    void receive_next_datagram_from(asio::ip::udp::endpoint& sender_endpoint
                                  , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, ecaludp::Error)>& completion_handler);

    std::shared_ptr<ecaludp::OwningBuffer> handle_datagram(const std::shared_ptr<ecaludp::RawMemory>&     buffer
                                                        , const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint
                                                        , ecaludp::Error&                                 error);

  /////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////
  private:
    std::unique_ptr<ecaludp::AsyncUdpcapSocket> socket_;                        ///< The "socket" implementation

    std::unique_ptr<recycle_shared_pool>      datagram_buffer_pool_;            ///< A reusable buffer pool for single datagrams (i.e. tyically 1500 byte fragments)
    std::unique_ptr<ecaludp::v5::Reassembly>  reassembly_v5_;                   ///< The reassembly for the eCAL v5 protocol

    std::array<char, 4>                       magic_header_bytes_;              ///< The magic bytes that are expected to start each fragment. If the received datagram doesn't have those, it will be dropped immediatelly
    std::chrono::steady_clock::duration       max_reassembly_age_;              ///< Fragments that are stored in the reassembly for longer than that period will be dropped.
  };
}