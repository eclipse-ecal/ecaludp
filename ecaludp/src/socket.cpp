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
#include <array>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <recycle/shared_pool.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include <asio.hpp> // IWYU pragma: keep

#include "ecaludp/error.h"
#include "ecaludp/raw_memory.h"
#include "protocol/datagram_builder_v5.h"
#include "protocol/datagram_description.h"
#include "protocol/header_common.h"
#include "protocol/reassembly_v5.h"

#include <ecaludp/owning_buffer.h>
#include <ecaludp/socket.h>

namespace ecaludp
{
  namespace
  {
    void async_send_datagram_list_to(asio::ip::udp::socket& socket
                                      , const DatagramList& datagram_list
                                      , DatagramList::const_iterator start_it
                                      , const asio::ip::udp::endpoint& destination
                                      , const std::function<void(asio::error_code)>& completion_handler)
    {
      if (start_it == datagram_list.end())
      {
        completion_handler(asio::error_code());
        return;
      }

      socket.async_send_to(start_it->asio_buffer_list_
                            , destination
                            , [&socket, &datagram_list, start_it, destination, completion_handler](asio::error_code ec, std::size_t /*bytes_transferred*/)
                              {
                                if (ec)
                                {
                                  completion_handler(ec);
                                  return;
                                }

                                async_send_datagram_list_to(socket, datagram_list, start_it + 1, destination, completion_handler);
                              });
    }
  }

  struct buffer_pool_lock_policy_
  {
    using mutex_type = std::mutex;
    using lock_type  = std::lock_guard<mutex_type>;
  };

  class recycle_shared_pool : public recycle::shared_pool<ecaludp::RawMemory, buffer_pool_lock_policy_>{};

  Socket::Socket(asio::io_context& io_context, std::array<char, 4> magic_header_bytes)
    : socket_               (io_context)
    , datagram_buffer_pool_ (std::make_unique<ecaludp::recycle_shared_pool>())
    , reassembly_v5_        (std::make_unique<ecaludp::v5::Reassembly>())
    , magic_header_bytes_   (magic_header_bytes)
    , max_udp_datagram_size_(1448)
    , max_reassembly_age_   (std::chrono::seconds(5))
  {}

  Socket::~Socket() = default;

  /////////////////////////////////////////////////////////////////
  // Sending
  /////////////////////////////////////////////////////////////////
  
  std::size_t Socket::send_to(const std::vector<asio::const_buffer>& buffer_sequence
                            , const asio::ip::udp::endpoint& destination
                            , asio::socket_base::message_flags flags
                            , asio::error_code& ec)
  {
    constexpr int protocol_version = 5;  //TODO: make this configurable

    DatagramList datagram_list;

    if (protocol_version == 5)
    {
      datagram_list = ecaludp::v5::create_datagram_list(buffer_sequence, max_udp_datagram_size_, magic_header_bytes_);
    }
    else
    {
      throw std::runtime_error("Protocol version not supported");
    }

    std::size_t sent(0);
    for (const auto& datagram : datagram_list)
    {
      sent += socket_.send_to(datagram.asio_buffer_list_, destination, flags, ec);
      if (ec) 
        break;
    }
    return sent;
  }

  void Socket::async_send_to(const std::vector<asio::const_buffer>& buffer_sequence
                                , const asio::ip::udp::endpoint& destination
                                , const std::function<void(asio::error_code)>& completion_handler)
  {
    constexpr int protocol_version  = 5;  //TODO: make this configurable

    auto datagram_list = std::make_shared<DatagramList>();

    if (protocol_version == 5)
    {
      *datagram_list = ecaludp::v5::create_datagram_list(buffer_sequence, max_udp_datagram_size_, magic_header_bytes_);
    }
    else
    {
      throw std::runtime_error("Protocol version not supported");
    }

    async_send_datagram_list_to(socket_
                              , *datagram_list
                              , datagram_list->begin()
                              , destination
                              , [completion_handler, datagram_list](asio::error_code ec)
                                {
                                  completion_handler(ec);
                                });
  }

  void Socket::set_max_udp_datagram_size(std::size_t max_udp_datagram_size)
  {
    max_udp_datagram_size_ = max_udp_datagram_size;
  }

  std::size_t Socket::get_max_udp_datagram_size() const
  {
    return max_udp_datagram_size_;
  }

  void Socket::set_max_reassembly_age(std::chrono::steady_clock::duration max_reassembly_age)
  {
    max_reassembly_age_ = max_reassembly_age;
  }

  std::chrono::steady_clock::duration Socket::get_max_reassembly_age() const
  {
    return max_reassembly_age_;
  }

  /////////////////////////////////////////////////////////////////
  // Receiving
  /////////////////////////////////////////////////////////////////
  std::shared_ptr<ecaludp::OwningBuffer> Socket::receive_from(asio::ip::udp::endpoint& sender_endpoint
                                                            , asio::socket_base::message_flags flags
                                                            , asio::error_code& ec)
  {
    while (true)
    {
      auto datagram_buffer = datagram_buffer_pool_->allocate();
      datagram_buffer->resize(65535); // Max UDP datagram size

      auto buffer = datagram_buffer_pool_->allocate();
      buffer->resize(65535); // max datagram size

      auto sender_endpoint_of_this_datagram = std::make_shared<asio::ip::udp::endpoint>();

      const std::size_t bytes_received = socket_.receive_from(asio::buffer(buffer->data(), buffer->size())
                                                            , *sender_endpoint_of_this_datagram
                                                            , flags
                                                            , ec);

      if (ec)
      {
        return nullptr;
      }

      // TODO: Revise this code. I am not sure, if this should be here. But I see no other way to get this working on Linux.
      // 
      // The following code exists due to a Linux issue:
      // - On Linux, calling close() or cancel() does not cause the synchronous receive_from() to return at all.
      // - The only way to un-block this call is to call shutdown() on the socket.
      // - However, calling shutdown does NOT cause an error to be set. Thus, we have no idea here, whether we have received a 0-byte datagram or whether an external thread has called shutdown().
      // - After an external thread has called shutdown(), each subsequent call to receive_from() will return 0 bytes without blocking.
      // - This will cause a busy loop here
      // - Thus, we need to check whether we have received a 0-byte datagram and whether the sender endpoint is still the default endpoint and break the busy loop in this case
      // 
      // I see this code as a workaround.
      if ((bytes_received == 0) && (*sender_endpoint_of_this_datagram == asio::ip::udp::endpoint()))
      {
        return nullptr;
      }

      // resize the buffer to the actually received size
      buffer->resize(bytes_received);

      // Handle the datagram
      ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
      auto completed_package = this->handle_datagram(buffer, sender_endpoint_of_this_datagram, error);

      if (completed_package != nullptr)
      {
        sender_endpoint = *sender_endpoint_of_this_datagram;
        return completed_package;
      }

      // If the datagram did not complete a package, receive the next datagram
    }
  }

  void Socket::async_receive_from(asio::ip::udp::endpoint& sender_endpoint
                                      , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, asio::error_code)>& completion_handler)
  {
    receive_next_datagram_from(sender_endpoint, completion_handler);
  }


  void Socket::receive_next_datagram_from(asio::ip::udp::endpoint& sender_endpoint
                                              , const std::function<void(const std::shared_ptr<ecaludp::OwningBuffer>&, asio::error_code)>& completion_handler)
  {
    auto datagram_buffer = datagram_buffer_pool_->allocate();
    datagram_buffer->resize(65535); // Max UDP datagram size

    auto buffer = datagram_buffer_pool_->allocate();

    buffer->resize(65535); // max datagram size

    auto sender_endpoint_of_this_datagram = std::make_shared<asio::ip::udp::endpoint>();

    socket_.async_receive_from(asio::buffer(buffer->data(), buffer->size())
                              , *sender_endpoint_of_this_datagram
                              , [this, buffer, completion_handler, sender_endpoint_of_this_datagram, &sender_endpoint](const asio::error_code& ec, std::size_t bytes_received)
                                {
                                  if (ec)
                                  {
                                    completion_handler(nullptr, ec);
                                    return;
                                  }

                                  // resize the buffer to the actually received size
                                  buffer->resize(bytes_received);

                                  // Handle the datagram
                                  ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
                                  auto completed_package = this->handle_datagram(buffer, sender_endpoint_of_this_datagram, error);

                                  if (completed_package != nullptr)
                                  {
                                    sender_endpoint = *sender_endpoint_of_this_datagram;
                                    completion_handler(completed_package, ec);
                                  }
                                  else
                                  {
                                    // Receive the next datagram
                                    receive_next_datagram_from(sender_endpoint, completion_handler);
                                  }
                                });

  }

  std::shared_ptr<ecaludp::OwningBuffer> Socket::handle_datagram(const std::shared_ptr<ecaludp::RawMemory>& buffer
                                                                  , const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint
                                                                  , ecaludp::Error& error)
  {
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
