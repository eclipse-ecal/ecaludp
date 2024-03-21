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

namespace ecaludp
{
  namespace v5
  {
    enum class datagram_type_uint32t : uint32_t
    {
      datagram_type_unknown                 = 0,
      datagram_type_fragmented_message_info = 1, // former name: msg_type_header
      datagram_type_fragment                = 2, // former name: msg_type_content
      datagram_type_non_fragmented_message  = 3  // former name: msg_type_header_with_content
    };

    #pragma pack(push, 1)
    struct Header
    {
      char                  magic[4];

      uint8_t               version;   /// Header version. Must be 5 for this version 5 header
      uint8_t               reserved1; /// Must be sent as 0. The old implementation used this byte as 4-byte version (little endian), but never checked it. Thus, it may be used in the future.
      uint8_t               reserved2; /// Must be sent as 0. The old implementation used this byte as 4-byte version (little endian), but never checked it. Thus, it may be used in the future.
      uint8_t               reserved3; /// Must be sent as 0. The old implementation used this byte as 4-byte version (little endian), but never checked it. Thus, it may be used in the future.

      datagram_type_uint32t type;      /// The datagram type. See datagram_type_uint32t for possible values

      int32_t               id;        /// Random ID to match fragmented parts of a message (Little-endian). Used differently depending on the datagram type:
                                       ///   - datagram_type_fragmented_message_info: The Random ID that this fragmentation info will be applied to
                                       ///   - datagram_type_fragment: The Random ID that this fragment belongs to. Used to match fragments to their fragmentation info
                                       ///   - datagram_type_non_fragmented_message: Unused field. Must be sent as -1. Must not be evaluated.

      uint32_t              num;       /// Fragment number (Little-endian). Used differently depending on the datagram type:
                                       ///   - datagram_type_fragmented_message_info: Amount of fragments that this message was split into.
                                       ///   - datagram_type_fragment: The number of this fragment
                                       ///   - datagram_type_non_fragmented_message: Unused field. Must be sent as 1. Must not be evaluated.

      uint32_t              len;       /// Payload length (Little-endian). Used differently depending on the datagram type. The payload must start directly after the header.
                                       ///   - datagram_type_fragmented_message_info: Length of the original message before it got fragmented.
                                       ///                                       Messages of this type must not carry any payload themselves.
                                       ///   - datagram_type_fragment: The payload lenght of this fragment
                                       ///   - datagram_type_non_fragmented_message: The payload length of this message
    };
    #pragma pack(pop)
  }
}