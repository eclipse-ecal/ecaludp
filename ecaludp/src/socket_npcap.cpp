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

#include <ecaludp/socket_npcap.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <ecaludp/error.h>
#include <ecaludp/owning_buffer.h>
#include <ecaludp/raw_memory.h>

#include <udpcap/host_address.h>

#include "async_udpcap_socket.h"

#include "protocol/header_common.h"
#include "protocol/reassembly_v5.h"

#include <recycle/shared_pool.hpp>

namespace ecaludp
{
  
  struct buffer_pool_lock_policy_
  {
    using mutex_type = std::mutex;
    using lock_type  = std::lock_guard<mutex_type>;
  };

  class recycle_shared_pool : public recycle::shared_pool<ecaludp::RawMemory, buffer_pool_lock_policy_>{};

  /////////////////////////////////////////////////////////////////
  // Constructor
  /////////////////////////////////////////////////////////////////
  SocketNpcap::SocketNpcap(std::array<char, 4> magic_header_bytes)
    : socket_              (std::make_unique<ecaludp::AsyncUdpcapSocket>())
    , datagram_buffer_pool_(std::make_unique<recycle_shared_pool>())
    , reassembly_v5_       (std::make_unique<ecaludp::v5::Reassembly>())
    , magic_header_bytes_  (magic_header_bytes)
    , max_reassembly_age_  (std::chrono::seconds(5))
  {}

  // Destructor
  SocketNpcap::~SocketNpcap() = default;

  /////////////////////////////////////////////////////////////////
  // Settings
  /////////////////////////////////////////////////////////////////
  void SocketNpcap::set_max_reassembly_age(std::chrono::steady_clock::duration max_reassembly_age)
  {
    max_reassembly_age_ = max_reassembly_age;
  }

  std::chrono::steady_clock::duration SocketNpcap::get_max_reassembly_age() const
  {
    return max_reassembly_age_;
  }

  /////////////////////////////////////////////////////////////////
  // API "Passthrough" (and a bit conversion to asio types)
  /////////////////////////////////////////////////////////////////
  bool SocketNpcap::is_valid() const                                                 { return socket_->isValid(); }
  bool SocketNpcap::bind(const asio::ip::udp::endpoint& sender_endpoint)             { return socket_->bind(Udpcap::HostAddress(sender_endpoint.address().to_string()), sender_endpoint.port()); }
  bool SocketNpcap::is_bound() const                                                 { return socket_->isBound(); }
  asio::ip::udp::endpoint SocketNpcap::local_endpoint()                              { return asio::ip::udp::endpoint(asio::ip::make_address(socket_->localAddress().toString()), socket_->localPort()); }
  bool SocketNpcap::set_receive_buffer_size(int size)                                { return socket_->setReceiveBufferSize(size); }
  bool SocketNpcap::join_multicast_group(const asio::ip::address_v4& group_address)  { return socket_->joinMulticastGroup(Udpcap::HostAddress(group_address.to_string())); }
  bool SocketNpcap::leave_multicast_group(const asio::ip::address_v4& group_address) { return socket_->leaveMulticastGroup(Udpcap::HostAddress(group_address.to_string())); }
  void SocketNpcap::set_multicast_loopback_enabled(bool enabled)                     { socket_->setMulticastLoopbackEnabled(enabled); }
  bool SocketNpcap::is_multicast_loopback_enabled() const                            { return socket_->isMulticastLoopbackEnabled(); }
  void SocketNpcap::close()                                                          { socket_->close(); }

  /////////////////////////////////////////////////////////////////
  // Receiving
  /////////////////////////////////////////////////////////////////
  
  std::shared_ptr<ecaludp::OwningBuffer> SocketNpcap::receive_from(asio::ip::udp::endpoint& sender_endpoint, ecaludp::Error& error)
  {
    while (true)
    {
      auto datagram_buffer = datagram_buffer_pool_->allocate();
      datagram_buffer->resize(65535); // Max UDP datagram size

      auto buffer = datagram_buffer_pool_->allocate();
      buffer->resize(65535); // max datagram size

      auto sender_address = std::make_shared<Udpcap::HostAddress>();
      auto sender_port    = std::make_shared<uint16_t>();

      size_t bytes_received = socket_->receiveFrom(reinterpret_cast<char*>(buffer->data())
                                                  , buffer->size()
                                                  , *sender_address
                                                  , *sender_port
                                                  , error);

      if (error)
      {
        return nullptr;
      }

      // resize the buffer to the actually received size
      buffer->resize(bytes_received);

      // Convert sender address and port to asio
      auto sender_endpoint_of_this_datagram = std::make_shared<asio::ip::udp::endpoint>(asio::ip::make_address(sender_address->toString()), *sender_port);

      // Handle the datagram. Discard the error, as we don't really want to
      // react on faulty datagrams here. Those will just be dropped and we will
      // continue to receive the next one.
      ecaludp::Error handle_datagram_error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
      auto completed_package = this->handle_datagram(buffer, sender_endpoint_of_this_datagram, handle_datagram_error);

      if (completed_package != nullptr)
      {
        sender_endpoint = *sender_endpoint_of_this_datagram;
        return completed_package;
      }
    }
  }
  
  
  void SocketNpcap::async_receive_from(asio::ip::udp::endpoint& sender_endpoint
                                      , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, const ecaludp::Error&)>& completion_handler)
  {
    receive_next_datagram_from(sender_endpoint, completion_handler);
  }

  void SocketNpcap::receive_next_datagram_from(asio::ip::udp::endpoint& sender_endpoint
                                              , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, const ecaludp::Error&)>& completion_handler)

  {
    auto datagram_buffer = datagram_buffer_pool_->allocate();
    datagram_buffer->resize(65535); // Max UDP datagram size

    auto buffer = datagram_buffer_pool_->allocate();

    buffer->resize(65535); // max datagram size

    auto sender_address = std::make_shared<Udpcap::HostAddress>();
    auto sender_port    = std::make_shared<uint16_t>();

    socket_->asyncReceiveFrom(reinterpret_cast<char*>(buffer->data())
                            , buffer->size()
                            , *sender_address
                            , *sender_port
                            , [this, buffer, completion_handler, sender_address, sender_port, &sender_endpoint](const ecaludp::Error& error, size_t bytes_received)
                              {
                                if (error)
                                {
                                  completion_handler(nullptr, error);
                                  return;
                                }
                                // resize the buffer to the actually received size
                                buffer->resize(bytes_received);

                                // Convert sender address and port to asio
                                auto sender_endpoint_of_this_datagram = std::make_shared<asio::ip::udp::endpoint>(asio::ip::make_address(sender_address->toString()), *sender_port);

                                // Handle the datagram
                                ecaludp::Error datagam_handle_error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
                                auto completed_package = this->handle_datagram(buffer, sender_endpoint_of_this_datagram, datagam_handle_error);

                                if (completed_package != nullptr)
                                {
                                  sender_endpoint = *sender_endpoint_of_this_datagram;
                                  completion_handler(completed_package, datagam_handle_error);
                                }
                                else
                                {
                                  // Receive the next datagram
                                  receive_next_datagram_from(sender_endpoint, completion_handler);
                                }
                              });

  }

  std::shared_ptr<ecaludp::OwningBuffer> SocketNpcap::handle_datagram(const std::shared_ptr<ecaludp::RawMemory>& buffer
                                                                      , const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint
                                                                      , ecaludp::Error& error)
  {
    // TODO: This function is code duplication.

    // Clean the reassembly from fragments that are too old
    reassembly_v5_->remove_old_packages(std::chrono::steady_clock::now() - max_reassembly_age_);

    // Start to parse the header

    if (buffer->size() < sizeof(ecaludp::HeaderCommon)) // Magic number + version
    {
      error = ecaludp::Error(ecaludp::Error::MALFORMED_DATAGRAM, "Datagram too small to contain common header (" + std::to_string(buffer->size()) + " bytes)");
      return nullptr;
    }

    auto* header = reinterpret_cast<ecaludp::HeaderCommon*>(buffer->data());

    // Check the magic number
    if (strncmp(header->magic, magic_header_bytes_.data(), 4) != 0)
    {
      error = ecaludp::Error(ecaludp::Error::MALFORMED_DATAGRAM, "Wrong magic bytes");
      return nullptr;
    }

    std::shared_ptr<ecaludp::OwningBuffer> finished_package;

    // Check the version and invoke the correct handler
    if (header->version == 5)
    {
      finished_package = reassembly_v5_->handle_datagram(buffer, sender_endpoint, error);
    }
    else if (header->version == 6)
    {
      error = ecaludp::Error(Error::UNSUPPORTED_PROTOCOL_VERSION, std::to_string(header->version));
      //handle_datagram_v6(buffer);
    }
    else
    {
      error = ecaludp::Error(Error::UNSUPPORTED_PROTOCOL_VERSION, std::to_string(header->version));
    }

    if (error)
    {
      return nullptr;
    }

    return finished_package;
  }

}