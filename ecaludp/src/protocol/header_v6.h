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

#include <cstdint>

// TODO: Add V6
namespace ecaludp
{
  namespace v6
  {
    #pragma pack(push, 1)
    struct Header
    {
      unsigned char magic[4];

      uint8_t version     = 0;
      uint8_t header_size = 0;
      uint8_t flags       = 0;
      uint8_t reserved    = 0;

      uint32_t package_id = 0;
    };

    struct FragmentHeader
    {
      uint32_t total_length    = 0;
      uint32_t fragment_offset = 0;
    };
    #pragma pop
  }
}