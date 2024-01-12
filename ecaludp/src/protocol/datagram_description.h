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

#include <cstddef>
#include <vector>

#include <asio.hpp>

namespace ecaludp {
class DatagramDescription
{
  public:
    // Default constructor
    DatagramDescription() = default;

    // Delete Copy constructor and assignment operator
    DatagramDescription(const DatagramDescription&) = delete;
    DatagramDescription& operator=(const DatagramDescription&) = delete;

    // Default Move constructor and assignment operator
    DatagramDescription(DatagramDescription&&) = default;
    DatagramDescription& operator=(DatagramDescription&&) = default;

    ~DatagramDescription() = default;

  public:
    std::vector<char>               header_buffer_   {};
    std::vector<asio::const_buffer> asio_buffer_list_{};

    size_t size() const
    {
      size_t size = 0;
      for (const auto& buffer : asio_buffer_list_)
      {
        size += buffer.size();
      }
      return size;
    }
  };

  using DatagramList = std::vector<DatagramDescription>;
  } // namespace ecaludp
