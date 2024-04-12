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

#include "datagram_description.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <asio.hpp> // IWYU pragma: keep

namespace ecaludp
{
  namespace v5
  {
    DatagramList create_datagram_list(const std::vector<asio::const_buffer>& buffer_sequence, size_t max_datagram_size, std::array<char, 4> magic_header_bytes);

    DatagramDescription create_non_fragmented_datagram(const std::vector<asio::const_buffer>& buffer_sequence, std::array<char, 4> magic_header_bytes);

    DatagramList create_fragmented_datagram_list(const std::vector<asio::const_buffer>& buffer_sequence, size_t max_udp_datagram_size, std::array<char, 4> magic_header_bytes);
  
    inline uint32_t xorshf96(uint32_t& x, uint32_t& y, uint32_t& z)
    {
      uint32_t t = 0;
      x ^= x << 16;
      x ^= x >> 5;
      x ^= x << 1;

      t = x;
      x = y;
      y = z;
      z = t ^ x ^ y;

      return z;
    }
  }
}
