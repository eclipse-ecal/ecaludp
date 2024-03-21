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
#include "reassembly_v5.h"

#include "ecaludp/owning_buffer.h"
#include "ecaludp/raw_memory.h"
#include "header_v5.h"
#include "portable_endian.h"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ecaludp/error.h>
#include <memory>
#include <string>

namespace ecaludp
{
  namespace v5
  {
    //////////////////////////////////////////////////////////////////////////////
    // Constructor, Destructor
    //////////////////////////////////////////////////////////////////////////////
    Reassembly::Reassembly() = default;

    //////////////////////////////////////////////////////////////////////////////
    // Receiving, datagram handling & fragment reassembly
    //////////////////////////////////////////////////////////////////////////////

    std::shared_ptr<ecaludp::OwningBuffer> Reassembly::handle_datagram(const std::shared_ptr<ecaludp::RawMemory>& buffer, const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint, ecaludp::Error& error)
    {
      if (buffer->size() < sizeof(ecaludp::v5::Header))
      {
        error = ecaludp::Error(ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM, "Datagram too small, cannot contain V5 header. Size is " + std::to_string(buffer->size()) + " bytes.");
        return nullptr;
      }

      const auto* header = reinterpret_cast<ecaludp::v5::Header*>(buffer->data());

      // Each message type must be handled differently
      if (static_cast<ecaludp::v5::datagram_type_uint32t>(le32toh(static_cast<uint32_t>(header->type)))
                 == ecaludp::v5::datagram_type_uint32t::datagram_type_fragmented_message_info)
      {
        return handle_datagram_fragmented_message_info(buffer, sender_endpoint, error);
      }
      else if (static_cast<ecaludp::v5::datagram_type_uint32t>(le32toh(static_cast<uint32_t>(header->type)))
                 == ecaludp::v5::datagram_type_uint32t::datagram_type_fragment)
      {
        return handle_datagram_fragment(buffer, sender_endpoint, error);
      }
      else if (static_cast<ecaludp::v5::datagram_type_uint32t>(le32toh(static_cast<uint32_t>(header->type)))
                 == ecaludp::v5::datagram_type_uint32t::datagram_type_non_fragmented_message)
      {
        return handle_datagram_non_fragmented_message(buffer, error);
      }
      else 
      {
        error = ecaludp::Error(ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM, "Invalid type");
        return nullptr;
      }
    }

    std::shared_ptr<ecaludp::OwningBuffer> Reassembly::handle_datagram_fragmented_message_info(const std::shared_ptr<ecaludp::RawMemory>& buffer, const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint, ecaludp::Error& error)
    {
      auto* header = reinterpret_cast<ecaludp::v5::Header*>(buffer->data());

      const int32_t package_id = le32toh(header->id);
      const fragmented_package_key package_key{*sender_endpoint, package_id};

      // Check if we already have a package with this id. If not, create one
      auto existing_package_it = fragmented_packages_.find(package_key);
      if (existing_package_it == fragmented_packages_.end())
      {
        existing_package_it = fragmented_packages_.emplace(package_key, fragmented_package{}).first;

      }
      else if (existing_package_it->second.first.fragment_info_received_)
      {
        error = ecaludp::Error(ecaludp::Error::ErrorCode::DUPLICATE_DATAGRAM
                                        , "Received fragment info for package " + std::to_string(package_id) + " twice");
        return nullptr;
      }

      // Store that we received the fragment info
      existing_package_it->second.first.fragment_info_received_ = true;

      // Set the fragmentation info
      existing_package_it->second.first.total_fragments_  = le32toh(header->num);
      existing_package_it->second.first.total_size_bytes_ = le32toh(header->len);

      // Resize the list of fragments, so we never have to resize again
      existing_package_it->second.second.resize(existing_package_it->second.first.total_fragments_);

      // Set the last access time
      existing_package_it->second.first.last_access_ = std::chrono::steady_clock::now();

      // Maybe the message is already complete. So let's check and reassemble the
      // package if necessary
      return handle_fragmented_package_if_complete(existing_package_it, error);
    }

    std::shared_ptr<ecaludp::OwningBuffer> Reassembly::handle_datagram_fragment(const std::shared_ptr<ecaludp::RawMemory>& buffer, const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint, ecaludp::Error& error)
    {
       auto* header = reinterpret_cast<ecaludp::v5::Header*>(buffer->data());

      const int32_t package_id = le32toh(header->id);
      const fragmented_package_key package_key{*sender_endpoint, package_id};

      // Check if we already have a package with this id. If not, create one
      auto existing_package_it = fragmented_packages_.find(package_key);
      if (existing_package_it == fragmented_packages_.end())
      {
        existing_package_it = fragmented_packages_.emplace(package_key, fragmented_package{}).first;
      }

      const uint32_t package_num = le32toh(header->num);
    
      // Resize the list of fragments, if necessary. We only do that, if we didn't
      // receive the fragment info yet, so we don't know how many fragments there
      // will be, yet
      if (!existing_package_it->second.first.fragment_info_received_)
      {
        existing_package_it->second.second.resize(std::max(existing_package_it->second.second.size(), static_cast<size_t>(package_num + 1)));
      }

      // Check if this fragment number fits in the list of fragments.
      if (package_num >= static_cast<uint32_t>(existing_package_it->second.second.size()))
      {
        error = ecaludp::Error(ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM
                                , "Fragment number " + std::to_string(package_num) + " is invalid. Should be smaller than " + std::to_string(existing_package_it->second.second.size()));
        return nullptr;
      }

      // Check if we already received this fragment
      if (existing_package_it->second.second[package_num] != nullptr)
      {
        error = ecaludp::Error(ecaludp::Error::ErrorCode::DUPLICATE_DATAGRAM
                                , "Fragment " + std::to_string(package_num) + " for package " + std::to_string(package_id));
        return nullptr;
      }
    
      // Check if the size information from the fragment is valid
      const uint32_t fragment_size = le32toh(header->len);
      const unsigned int bytes_available = (static_cast<unsigned int>(buffer->size()) - sizeof(ecaludp::v5::Header));
      if (fragment_size > bytes_available)
      {
        error = ecaludp::Error(ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM
                                       , "Faulty size of fragment. Should be " + std::to_string(fragment_size)
                                          + ", but only " + std::to_string(bytes_available) + " bytes availabe.");
        return nullptr;
      }

      // prepare a buffer view to the payload data and store the fragment in the list
      const void* payload_data_ptr = static_cast<const uint8_t*>(buffer->data()) + sizeof(ecaludp::v5::Header);
      auto fragment_buffer_view = std::make_shared<ecaludp::OwningBuffer>(payload_data_ptr, static_cast<size_t>(fragment_size), buffer);
      existing_package_it->second.second[package_num] = fragment_buffer_view;

      // Increase the number of received fragments
      existing_package_it->second.first.received_fragments_++;

      // Set the last access time
      existing_package_it->second.first.last_access_ = std::chrono::steady_clock::now();

      // Maybe the message is already complete. So let's check and reassemble the
      // package if necessary
      return handle_fragmented_package_if_complete(existing_package_it, error);
    }

    std::shared_ptr<ecaludp::OwningBuffer> Reassembly::handle_datagram_non_fragmented_message(const std::shared_ptr<ecaludp::RawMemory>& buffer, ecaludp::Error& error)
    {
      const auto* header = reinterpret_cast<ecaludp::v5::Header*>(buffer->data());

      // Check if the size information from the header is valid
      const uint32_t payload_size = le32toh(header->len);
      const unsigned int bytes_available = (static_cast<unsigned int>(buffer->size()) - sizeof(ecaludp::v5::Header));

      if (payload_size > bytes_available)
      {
        error = ecaludp::Error(ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM
                               , "Faulty size of datagram. Should be " + std::to_string(payload_size)
                                  + ", but only " + std::to_string(bytes_available) + " bytes availabe.");
        return nullptr;
      }

      // Calculate the pointer to the payload data and create an OwningBuffer for that memory area
      const void* payload_data_ptr = static_cast<const uint8_t*>(buffer->data()) + sizeof(ecaludp::v5::Header);
      auto payload_buffer = std::make_shared<ecaludp::OwningBuffer>(payload_data_ptr, static_cast<size_t>(payload_size), buffer);

      error = ecaludp::Error::ErrorCode::OK;
      return payload_buffer;
    }

    std::shared_ptr<ecaludp::OwningBuffer> Reassembly::handle_fragmented_package_if_complete(const fragmented_package_map_t::const_iterator& it, ecaludp::Error& error)
    {
      // Check if we have a completed package
      if (!(it->second.first.fragment_info_received_)
            || !(it->second.first.received_fragments_ == it->second.first.total_fragments_))
      {
        error = ecaludp::Error::ErrorCode::OK;
        return nullptr;
      }

      // Check if the package size is valid
      {
        size_t cummulated_package_sizes = 0;
        for (const auto& fragment : it->second.second)
        {
          cummulated_package_sizes += fragment->size();
        }

        if (cummulated_package_sizes != it->second.first.total_size_bytes_)
        {
          // TODO: Add a performance option that doesn't build the long error string.
          error = ecaludp::Error(Error::ErrorCode::MALFORMED_REASSEMBLED_MESSAGE
                          , "Size error. Should be " + std::to_string(it->second.first.total_size_bytes_)
                            + " bytes, but received " + std::to_string(cummulated_package_sizes) + "bytes.");

          // Remove the package from the map. We don't need it anymore, as it is corrupted
          fragmented_packages_.erase(it);

          return nullptr;
        }
      }

      // We have a complete package, so we can reassemble it
      auto reassebled_buffer = reassemble_package(it);

      // Remove the package from the map. We don't need it anymore, as it is complete
      fragmented_packages_.erase(it);

      // Return the package to the user
      error = ecaludp::Error::ErrorCode::OK;
      return reassebled_buffer;
    }

    std::shared_ptr<ecaludp::OwningBuffer> Reassembly::reassemble_package(const fragmented_package_map_t::const_iterator& it)
    {
      // Create a mutable buffer that is big enough to hold the entire package
      auto reassembled_buffer = largepackage_buffer_pool_.allocate();

      reassembled_buffer->resize(it->second.first.total_size_bytes_);

      void* current_pos = reassembled_buffer->data();

      for (const auto& fragment : it->second.second)
      {
        // Copy the fragment into the reassembled buffer
        memcpy(current_pos, fragment->data(), fragment->size());
        current_pos = static_cast<uint8_t*>(current_pos) + fragment->size();
      }

      // In this case we don't have the header as residue in the raw memory, so we return the entire buffer.
      return std::make_shared<ecaludp::OwningBuffer>(reassembled_buffer->data(), reassembled_buffer->size(), reassembled_buffer);
    }

    void Reassembly::remove_old_packages(std::chrono::steady_clock::time_point max_age)
    {
      // Remove all packages that are older than max_age
      for (auto it = fragmented_packages_.begin(); it != fragmented_packages_.end();)
      {
        if (it->second.first.last_access_ < max_age)
        {
          fragmented_packages_.erase(it++);
        }
        else
        {
          ++it;
        }
      }
    }

  }
}
