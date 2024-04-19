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
#include <cstddef>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>
#include <vector>

#include <asio.hpp>

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
  class AsyncUdpcapSocket;

  class SocketNpcap
  {
  /////////////////////////////////////////////////////////////////
  // Constructor
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT SocketNpcap(std::array<char, 4> magic_header_bytes);

    // Destructor
    ECALUDP_EXPORT ~SocketNpcap();

    // Disable copy constructor and assignment operator
    SocketNpcap(const SocketNpcap&)             = delete;
    SocketNpcap& operator=(const SocketNpcap&)  = delete;

    // Disable move constructor and assignment operator
    SocketNpcap(SocketNpcap&&)            = delete;
    SocketNpcap& operator=(SocketNpcap&&) = delete;

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
    ECALUDP_EXPORT bool is_valid() const;
    ECALUDP_EXPORT bool bind(const asio::ip::udp::endpoint& sender_endpoint);
    ECALUDP_EXPORT bool is_bound() const;
    ECALUDP_EXPORT asio::ip::udp::endpoint local_endpoint();
    ECALUDP_EXPORT bool set_receive_buffer_size(int size);
    ECALUDP_EXPORT bool join_multicast_group(const asio::ip::address_v4& group_address);
    ECALUDP_EXPORT bool leave_multicast_group(const asio::ip::address_v4& group_address);
    ECALUDP_EXPORT void set_multicast_loopback_enabled(bool enabled);
    ECALUDP_EXPORT bool is_multicast_loopback_enabled() const;
    ECALUDP_EXPORT void close();

  /////////////////////////////////////////////////////////////////
  // Receiving
  /////////////////////////////////////////////////////////////////
  public:
    ECALUDP_EXPORT std::shared_ptr<ecaludp::OwningBuffer> receive_from(asio::ip::udp::endpoint& sender_endpoint, ecaludp::Error& error);

    ECALUDP_EXPORT void async_receive_from(asio::ip::udp::endpoint& sender_endpoint
                                  , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, const ecaludp::Error&)>& completion_handler);

  private:
    void receive_next_datagram_from(asio::ip::udp::endpoint& sender_endpoint
                                  , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, const ecaludp::Error&)>& completion_handler);

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