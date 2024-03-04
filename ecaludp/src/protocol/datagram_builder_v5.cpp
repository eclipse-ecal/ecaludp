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

#include "datagram_builder_v5.h"

#include "header_v5.h"
#include "portable_endian.h"
#include "protocol/datagram_description.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace ecaludp
{
  namespace v5
  {

    DatagramList create_datagram_list(const std::vector<asio::const_buffer>& buffer_sequence, size_t max_datagram_size, std::array<char, 4> magic_header_bytes)
    {
      // TODO: Complain when the max_udp_datagram_size is too small (the header doesn't even fit)

      constexpr size_t header_size = sizeof(ecaludp::v5::Header);

      // Create a new buffer_sequence that doesn't contain zero-sized buffers
      std::vector<asio::const_buffer> buffer_sequence_without_zero_sized_buffers;
      buffer_sequence_without_zero_sized_buffers.reserve(buffer_sequence.size());
      for (const auto& buffer : buffer_sequence)
      {
        if (buffer.size() > 0)
        {
        buffer_sequence_without_zero_sized_buffers.push_back(buffer);
        }
      }

      // Calculate the total size of all buffers
      size_t total_size = 0;
      for (const auto& buffer : buffer_sequence_without_zero_sized_buffers)
      {
        total_size += buffer.size();
      }

      if ((total_size + header_size) <= max_datagram_size)
      {
        // Small enough! We can send the entire payload in one datagram
        DatagramList datagram_list;
        datagram_list.reserve(1);
        datagram_list.emplace_back(create_non_fragmented_datagram(buffer_sequence_without_zero_sized_buffers, magic_header_bytes));

        return datagram_list;
      }
      else
      {
        // Too big! We need to fragment the payload
        return create_fragmented_datagram_list(buffer_sequence_without_zero_sized_buffers, max_datagram_size, magic_header_bytes);
      }
    }

    DatagramDescription create_non_fragmented_datagram(const std::vector<asio::const_buffer>& buffer_sequence, std::array<char, 4> magic_header_bytes)
    {
      uint32_t total_size = 0;
      for (const auto& buffer : buffer_sequence)
      {
        total_size += static_cast<uint32_t>(buffer.size());
      }

      // Container for the header buffer and all asio buffers
      DatagramDescription datagram_description;

      // Create one buffer for the header. That buffer will be the only additional
      // needed buffer.
      datagram_description.header_buffer_.resize(sizeof(ecaludp::v5::Header));

      // Fill the header
      auto* header_ptr = reinterpret_cast<ecaludp::v5::Header*>(datagram_description.header_buffer_.data());

      header_ptr->magic[0] = magic_header_bytes[0];
      header_ptr->magic[1] = magic_header_bytes[1];
      header_ptr->magic[2] = magic_header_bytes[2];
      header_ptr->magic[3] = magic_header_bytes[3];

      header_ptr->version = 5;

      header_ptr->type     = static_cast<ecaludp::v5::message_type_uint32t>(
                                htole32(static_cast<uint32_t>(ecaludp::v5::message_type_uint32t::msg_type_non_fragmented_message)));
      header_ptr->id       = htole32(int32_t(-1));   // -1 => not fragmented
      header_ptr->num      = htole32(uint32_t(1));    // 1 => only 1 fragment
      header_ptr->len      = htole32(static_cast<uint32_t>(total_size)); // denotes the length of the payload of this message only

      // Add an asio buffer for the header buffer
      datagram_description.asio_buffer_list_.emplace_back(datagram_description.header_buffer_.data(), datagram_description.header_buffer_.size());

      // Add an asio buffer for each payload buffer
      for (const auto& buffer : buffer_sequence)
      {
        datagram_description.asio_buffer_list_.emplace_back(buffer.data(), buffer.size());
      }

      return datagram_description;
    }

    DatagramList create_fragmented_datagram_list(const std::vector<asio::const_buffer>& buffer_sequence, size_t max_udp_datagram_size, std::array<char, 4> magic_header_bytes)
    {
      // Count the total size of all buffers
      uint32_t total_size = 0;
      for (const auto& buffer : buffer_sequence)
      {
        total_size += static_cast<uint32_t>(buffer.size());
      }

      // Compute how many bytes we can send at once
      const int payload_bytes_per_datagram = static_cast<int>(max_udp_datagram_size) - sizeof(ecaludp::v5::Header);

      // Compute how many datagrams we need. We need 1 datagram more, as
      // the fragmentation info must be sent in a separate datagram
      const uint32_t needed_fragment_count = ((total_size + (payload_bytes_per_datagram - 1)) / payload_bytes_per_datagram);
      const uint32_t needed_datagram_count = 1 + needed_fragment_count;

      // Pre-allocate the datagram list, so we never have to re-allocate
      DatagramList datagram_list;
      datagram_list.reserve(needed_datagram_count);

      // Create a random number for the package ID, that is used to match all fragments
      uint32_t message_id = 0;
      {
        static std::mutex mutex;
        const std::lock_guard<std::mutex> lock(mutex);
        static uint32_t x = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                std::chrono::high_resolution_clock::now().time_since_epoch()).count()
                                );
        static uint32_t y = 362436069;
        static uint32_t z = 521288629;
        message_id = xorshf96(x, y, z);
      }

      // Create the fragmentation info
      {
        datagram_list.emplace_back();
        datagram_list.back().header_buffer_.resize(sizeof(ecaludp::v5::Header));

        auto* fragment_info_header_ptr = reinterpret_cast<ecaludp::v5::Header*>(datagram_list.back().header_buffer_.data());

        fragment_info_header_ptr->magic[0] = magic_header_bytes[0];
        fragment_info_header_ptr->magic[1] = magic_header_bytes[1];
        fragment_info_header_ptr->magic[2] = magic_header_bytes[2];
        fragment_info_header_ptr->magic[3] = magic_header_bytes[3];

        fragment_info_header_ptr->version = 5;

        fragment_info_header_ptr->type     = static_cast<ecaludp::v5::message_type_uint32t>(
                                                htole32(static_cast<int32_t>(ecaludp::v5::message_type_uint32t::msg_type_fragmented_message_info)));
        fragment_info_header_ptr->id       = htole32(message_id);
        fragment_info_header_ptr->num      = htole32(needed_fragment_count);
        fragment_info_header_ptr->len      = htole32(total_size); // denotes the length of the entire payload

        // Add an asio buffer for the header buffer
        datagram_list.back().asio_buffer_list_.emplace_back(datagram_list.back().header_buffer_.data(), datagram_list.back().header_buffer_.size());
      }

      // Iterate over all buffers and create fragments for them
      size_t buffer_index             = 0;
      size_t offset_in_current_buffer = 0;

      while ((buffer_index < buffer_sequence.size()
            || ((buffer_index == (buffer_sequence.size() - 1))
                && (offset_in_current_buffer < buffer_sequence[buffer_index].size()))))
      {
        // Create a new datagram, if the last one is full, or if this is the first real datagram (besides the fragmentation info)
        if ((datagram_list.size() <= 1)
          || (datagram_list.back().size() >= max_udp_datagram_size))
        {
          datagram_list.emplace_back();
          datagram_list.back().header_buffer_.resize(sizeof(ecaludp::v5::Header));

          auto* const header_ptr = reinterpret_cast<ecaludp::v5::Header*>(datagram_list.back().header_buffer_.data());

          header_ptr->magic[0] = magic_header_bytes[0];
          header_ptr->magic[1] = magic_header_bytes[1];
          header_ptr->magic[2] = magic_header_bytes[2];
          header_ptr->magic[3] = magic_header_bytes[3];
          
          header_ptr->version = 5;

          header_ptr->type     = static_cast<ecaludp::v5::message_type_uint32t>(
                                    htole32(static_cast<uint32_t>(ecaludp::v5::message_type_uint32t::msg_type_fragment)));
          header_ptr->id       = htole32(message_id);
          header_ptr->num      = htole32(static_cast<uint32_t>(datagram_list.size() - 2)); // -1, because the first datagram is the fragmentation info
          header_ptr->len      = htole32(static_cast<uint32_t>(0));                        // denotes the length of the entire payload

          // Add an asio buffer for the header buffer
          datagram_list.back().asio_buffer_list_.emplace_back(datagram_list.back().header_buffer_.data(), datagram_list.back().header_buffer_.size());
        }

        // Compute how many bytes from the current buffer we can fit in the datagram
        const size_t bytes_to_fit_in_current_datagram = std::min(max_udp_datagram_size - datagram_list.back().size(), buffer_sequence[buffer_index].size() - offset_in_current_buffer);

        // Add an asio buffer to the datagram that points to the data in the user buffer
        datagram_list.back().asio_buffer_list_.emplace_back(reinterpret_cast<const uint8_t*>(buffer_sequence[buffer_index].data()) + offset_in_current_buffer, bytes_to_fit_in_current_datagram);

        // Increase the size of the current datagram
        auto* const header_ptr = reinterpret_cast<ecaludp::v5::Header*>(datagram_list.back().header_buffer_.data());
        header_ptr->len = htole32(le32toh(header_ptr->len) + static_cast<uint32_t>(bytes_to_fit_in_current_datagram));

        // Increase the offset in the user buffer
        offset_in_current_buffer += bytes_to_fit_in_current_datagram;

        // Check if we reached the end of the current user buffer
        if (offset_in_current_buffer >= buffer_sequence[buffer_index].size())
        {
          // Increase the buffer index
          buffer_index++;
    
          // Reset the offset in the current buffer
          offset_in_current_buffer = 0;
        }
      }

      // return the datagram list
      return datagram_list;
    }
  }
}
