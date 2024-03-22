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

#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <asio.hpp> // IWYU pragma: keep

#include <recycle/shared_pool.hpp>

#include <ecaludp/error.h>
#include <ecaludp/owning_buffer.h>
#include <ecaludp/raw_memory.h>

namespace ecaludp
{
  namespace v5
  {
    class Reassembly
    {
      //////////////////////////////////////////////////////////////////////////////
      // Private types
      //////////////////////////////////////////////////////////////////////////////
      private:
        using fragmented_package_key = std::pair<asio::ip::udp::endpoint, int32_t>;
        struct fragmented_package_info
        {
          bool                                  fragment_info_received_ {false};
          uint32_t                              total_fragments_        {0};
          uint32_t                              total_size_bytes_       {0};
          unsigned int                          received_fragments_     {0};
          std::chrono::steady_clock::time_point last_access_            {std::chrono::steady_clock::duration(0)};
        };
        using fragmented_package       = std::pair<fragmented_package_info, std::vector<std::shared_ptr<ecaludp::OwningBuffer>>>;
        using fragmented_package_map_t = std::map<fragmented_package_key, fragmented_package>;

    //////////////////////////////////////////////////////////////////////////////
    // Constructor, Destructor
    //////////////////////////////////////////////////////////////////////////////
    public:
      Reassembly();

    //////////////////////////////////////////////////////////////////////////////
    // Receiving, datagram handling & fragment reassembly
    //////////////////////////////////////////////////////////////////////////////
    public:
      std::shared_ptr<ecaludp::OwningBuffer> handle_datagram                        (const std::shared_ptr<ecaludp::RawMemory>& buffer, const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint, ecaludp::Error& error);

    private:
      std::shared_ptr<ecaludp::OwningBuffer> handle_datagram_fragmented_message_info(const std::shared_ptr<ecaludp::RawMemory>& buffer, const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint, ecaludp::Error& error);
      std::shared_ptr<ecaludp::OwningBuffer> handle_datagram_fragment               (const std::shared_ptr<ecaludp::RawMemory>& buffer, const std::shared_ptr<asio::ip::udp::endpoint>& sender_endpoint, ecaludp::Error& error);
      std::shared_ptr<ecaludp::OwningBuffer> handle_datagram_non_fragmented_message (const std::shared_ptr<ecaludp::RawMemory>& buffer, ecaludp::Error& error);

      std::shared_ptr<ecaludp::OwningBuffer> handle_fragmented_package_if_complete(const fragmented_package_map_t::const_iterator& it, ecaludp::Error& error);
      std::shared_ptr<ecaludp::OwningBuffer> reassemble_package  (const fragmented_package_map_t::const_iterator& it);

    public:
      void remove_old_packages(std::chrono::steady_clock::time_point max_age);

    //////////////////////////////////////////////////////////////////////////////
    // Member variables
    //////////////////////////////////////////////////////////////////////////////
    private:
      fragmented_package_map_t fragmented_packages_;

      // Buffer pool
      struct buffer_pool_lock_policy_
      {
        using mutex_type = std::mutex;
        using lock_type  = std::lock_guard<mutex_type>;
      };
      recycle::shared_pool<ecaludp::RawMemory, buffer_pool_lock_policy_> largepackage_buffer_pool_;
    };
  }
}
