/********************************************************************************
 * Copyright (c) 2024 Continental Corporation
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 * 
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

#include <asio.hpp> // IWYU pragma: keep

// IWYU pragma: begin_exports
#include <ecaludp/ecaludp_export.h>
#include <ecaludp/error.h>
#include <ecaludp/owning_buffer.h>
#include <ecaludp/raw_memory.h>
// IWYU pragma: end_exports

namespace ecaludp
{
  namespace v5
  {
    class Reassembly;
  }

  class recycle_shared_pool;

  class Socket
  {
  /////////////////////////////////////////////////////////////////
  // Constructor
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT Socket(asio::io_service& io_service, std::array<char, 4> magic_header_bytes);

    // Destructor
    ECALUDP_EXPORT ~Socket();

    // Disable copy constructor and assignment operator
    Socket(const Socket&)             = delete;
    Socket& operator=(const Socket&)  = delete;

    // Disable move constructor and assignment operator
    Socket(Socket&&)            = delete;
    Socket& operator=(Socket&&) = delete;

  /////////////////////////////////////////////////////////////////
  // API Passthrough
  /////////////////////////////////////////////////////////////////
  
    bool at_mark()                     const                                                     { return socket_.at_mark(); }
    bool at_mark(asio::error_code& ec) const                                                     { return socket_.at_mark(ec); }

    std::size_t available()                     const                                            { return socket_.available(); }
    std::size_t available(asio::error_code& ec) const                                            { return socket_.available(ec); }

    void bind(const asio::ip::udp::endpoint& endpoint)                                           { socket_.bind(endpoint); }
    asio::error_code bind(const asio::ip::udp::endpoint& endpoint, asio::error_code& ec)         { return socket_.bind(endpoint, ec); }

    void cancel()                                                                                { socket_.cancel(); }
    asio::error_code cancel(asio::error_code& ec)                                                { return socket_.cancel(ec); }

    void close()                                                                                 { socket_.close(); }
    asio::error_code close(asio::error_code& ec)                                                 { return socket_.close(ec); }

    void connect(const asio::ip::udp::endpoint& peer_endpoint)                                   { socket_.connect(peer_endpoint); }
    asio::error_code connect(const asio::ip::udp::endpoint& peer_endpoint, asio::error_code& ec) { return socket_.connect(peer_endpoint, ec); }

    const asio::any_io_executor& get_executor()                                                  { return socket_.get_executor(); }

    template<typename GettableSocketOption>
    void get_option(GettableSocketOption& option)                                                { socket_.get_option(option); }

    template<typename GettableSocketOption>
    asio::error_code get_option(GettableSocketOption& option, asio::error_code& ec)              { return socket_.get_option(option, ec); }

    template<typename IoControlCommand>
    void io_control(IoControlCommand& command)                                                   { socket_.io_control(command); }

    template<typename IoControlCommand>
    asio::error_code io_control(IoControlCommand& command, asio::error_code& ec)                 { return socket_.io_control(command, ec); }

    bool is_open() const                                                                         { return socket_.is_open(); }

    asio::ip::udp::endpoint local_endpoint()                     const                           { return socket_.local_endpoint(); }
    asio::ip::udp::endpoint local_endpoint(asio::error_code& ec) const                           { return socket_.local_endpoint(ec); }

    asio::ip::udp::socket::lowest_layer_type& lowest_layer()                                     { return socket_.lowest_layer(); }
    const asio::ip::udp::socket::lowest_layer_type& lowest_layer() const                         { return socket_.lowest_layer(); }

    asio::ip::udp::socket::native_handle_type native_handle()                                    { return socket_.native_handle(); }

    bool native_non_blocking() const                                                             { return socket_.native_non_blocking(); }
    void native_non_blocking(bool mode)                                                          { socket_.native_non_blocking(mode); }
    void native_non_blocking(bool mode, asio::error_code& ec)                                    { socket_.native_non_blocking(mode, ec); }

    void open(const asio::ip::udp& protocol)                                                     { socket_.open(protocol); }
    asio::error_code open(const asio::ip::udp& protocol, asio::error_code& ec)                   { return socket_.open(protocol, ec); }

    asio::ip::udp::endpoint remote_endpoint()                     const                          { return socket_.remote_endpoint(); }
    asio::ip::udp::endpoint remote_endpoint(asio::error_code& ec) const                          { return socket_.remote_endpoint(ec); }

    template<typename SettableSocketOption>
    void set_option(const SettableSocketOption& option)                                          { socket_.set_option(option); }

    template<typename SettableSocketOption>
    asio::error_code set_option(const SettableSocketOption& option, asio::error_code& ec)        { return socket_.set_option(option, ec); }

    void shutdown(asio::socket_base::shutdown_type what)                                         { socket_.shutdown(what); }
    asio::error_code shutdown(asio::socket_base::shutdown_type what, asio::error_code& ec)       { return socket_.shutdown(what, ec); }


  /////////////////////////////////////////////////////////////////
  // Sending
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT void async_send_to(const std::vector<asio::const_buffer>& buffer_sequence
                                    , const asio::ip::udp::endpoint& destination
                                    , const std::function<void(asio::error_code)>& completion_handler);

    ECALUDP_EXPORT void set_max_udp_datagram_size(std::size_t max_udp_datagram_size);
    ECALUDP_EXPORT std::size_t get_max_udp_datagram_size() const;

    ECALUDP_EXPORT void set_max_reassembly_age(std::chrono::steady_clock::duration max_reassembly_age);
    ECALUDP_EXPORT std::chrono::steady_clock::duration get_max_reassembly_age() const;
  /////////////////////////////////////////////////////////////////
  // Receiving
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT void async_receive_from(asio::ip::udp::endpoint& sender_endpoint
                                  , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, asio::error_code)>& completion_handler);

  private:
    void receive_next_datagram_from(asio::ip::udp::endpoint& sender_endpoint
                                  , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, asio::error_code)>& completion_handler);

    std::shared_ptr<ecaludp::OwningBuffer> handle_datagram(const std::shared_ptr<ecaludp::RawMemory>& buffer
                                                          , const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint
                                                          , ecaludp::Error& error);

  /////////////////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////////////////
  private:
    asio::ip::udp::socket                     socket_;
    std::unique_ptr<recycle_shared_pool>      datagram_buffer_pool_;
    std::unique_ptr<ecaludp::v5::Reassembly>  reassembly_v5_;

    std::array<char, 4>                       magic_header_bytes_;
    std::size_t                               max_udp_datagram_size_;
    std::chrono::steady_clock::duration       max_reassembly_age_;
  };
}
