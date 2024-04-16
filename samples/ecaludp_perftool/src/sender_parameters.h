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

#include <string>
#include <cstdint>
#include <sstream>

struct SenderParameters
{
  std::string ip                    { "127.0.0.1" };
  uint16_t    port                  {14000};
  size_t      message_size          {0};
  int         max_udp_datagram_size {-1};
  int         buffer_size           {-1};

  std::string to_string() const
  {
    std::stringstream ss;

    ss << "SenderParameters: " << std::endl;
    ss << "  ip:                    " << ip << std::endl;
    ss << "  port:                  " << port << std::endl;
    ss << "  message_size:          " << message_size << std::endl;
    ss << "  max_udp_datagram_size: " << (max_udp_datagram_size > 0 ? std::to_string(max_udp_datagram_size) : "default") << std::endl;
    ss << "  buffer_size:           " << (buffer_size > 0 ? std::to_string(buffer_size) : "default") << std::endl;

    return ss.str();
  }
};