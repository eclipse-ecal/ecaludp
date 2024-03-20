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

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

#include <asio.hpp>

#include <ecaludp/error.h>
#include <ecaludp/raw_memory.h>

#include <protocol/datagram_builder_v5.h>
#include <protocol/datagram_description.h>
#include <protocol/header_v5.h>
#include <protocol/portable_endian.h>
#include <protocol/reassembly_v5.h>

std::shared_ptr<ecaludp::RawMemory> to_binary_buffer(const ecaludp::DatagramDescription& datagram_description)
{
  std::shared_ptr<ecaludp::RawMemory> buffer = std::make_shared<ecaludp::RawMemory>();
  buffer->resize(datagram_description.size());

  size_t current_pos = 0;
  
  for (const auto& asio_buffer : datagram_description.asio_buffer_list_)
  {
    std::memcpy(buffer->data() + current_pos, asio_buffer.data(), asio_buffer.size());
    current_pos += asio_buffer.size();
  }

  return buffer;
}

// Check "Fragmentation" and "Defragmentation" of a single normal message that is smaller than the MTU, i.e. no fragmentation is needed
TEST(FragmentationV5Test, NonFragmentedMessage)
{
  // Create a Hello World string
  std::string hello_world = "Hello World!";

  // Create an asio buffer from the string
  asio::const_buffer hello_world_buffer = asio::buffer(hello_world);

  // Let the datagram builder create fragments for the buffer with a max datagram size of 1000 bytes (including header)
  auto datagram_list = ecaludp::v5::create_datagram_list({hello_world_buffer}, 1000, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 1 entry
  ASSERT_EQ(datagram_list.size(), 1);

  // The size of the datagram list is the size of the buffer plus the size of the header
  ASSERT_EQ(datagram_list.front().size(), hello_world.size() + sizeof(ecaludp::v5::Header));

  // Copy the datagram list to a binary buffer
  auto binary_buffer = to_binary_buffer(datagram_list.front());

  // Check the header
  auto* header = reinterpret_cast<ecaludp::v5::Header*>(binary_buffer->data());
  ASSERT_EQ(header->version, 5);
  ASSERT_EQ(le32toh(static_cast<uint32_t>(header->type)), 3u /* = ecaludp::v5::message_type_uint32t::msg_type_non_fragmented_message */);
  ASSERT_EQ(le32toh(header->id), -1);
  ASSERT_EQ(le32toh(header->num), 1);
  ASSERT_EQ(le32toh(header->len), hello_world.size());

  // Create a reassembly object
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassebly the datagram
  ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
  auto message = reassembly.handle_datagram(binary_buffer, sender_endpoint, error);

  // The reassembly must have succeeded
  ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);

  // The message must not be nullptr
  ASSERT_NE(message, nullptr);

  // The message must have the same size as the original buffer
  ASSERT_EQ(message->size(), hello_world.size());

  // The message must contain the same data as the original buffer
  ASSERT_EQ(std::memcmp(message->data(), hello_world.data(), hello_world.size()), 0);
}

// Check the fragmentation and defragmentation of a single message that is larger than the MTU
TEST(FragmentationV5Test, FragmentedMessage)
{
  // Create a longer string
  auto message_to_send = std::string("In the beginning the Universe was created. This had made many people very angry and has been widely regarded as a bad move.");
  const size_t message_size = message_to_send.size();

  // Create an asio buffer from the string
  asio::const_buffer message_to_send_buffer = asio::buffer(message_to_send);

  // Let the datagram builder create fragments for the buffer with a max datagram size of 100 bytes (including header)
  auto datagram_list = ecaludp::v5::create_datagram_list({message_to_send_buffer}, 100, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 3 entries: 1 fragment info and 2 fragments
  ASSERT_EQ(datagram_list.size(), 3);

  // The size of the datagram list is the size of the buffer plus the size of the header
  ASSERT_EQ(datagram_list[0].size(), sizeof(ecaludp::v5::Header)); // This is the fragment info
  ASSERT_EQ(datagram_list[1].size(), 100);                         // This is the entire full datagram
  ASSERT_EQ(datagram_list[2].size(), sizeof(ecaludp::v5::Header) + message_size - (100 - sizeof(ecaludp::v5::Header))); // This is the last fragment

  // Copy the datagram list to a couple of binary buffers
  auto binary_buffer_1 = to_binary_buffer(datagram_list[0]);
  auto binary_buffer_2 = to_binary_buffer(datagram_list[1]);
  auto binary_buffer_3 = to_binary_buffer(datagram_list[2]);

  // Check the header of the fragment info
  auto* header_1 = reinterpret_cast<ecaludp::v5::Header*>(binary_buffer_1->data());
  auto common_id = le32toh(header_1->id);
  ASSERT_EQ(header_1->version, 5);
  ASSERT_EQ(le32toh(static_cast<uint32_t>(header_1->type)), 1u /* = ecaludp::v5::message_type_uint32t::msg_type_fragment_info */);
  ASSERT_EQ(le32toh(header_1->num), 2);
  ASSERT_EQ(le32toh(header_1->id), common_id);
  ASSERT_EQ(le32toh(header_1->len), message_size);

  // Check the header of the first fragment
  auto* header_2 = reinterpret_cast<ecaludp::v5::Header*>(binary_buffer_2->data());
  ASSERT_EQ(header_2->version, 5);
  ASSERT_EQ(le32toh(static_cast<uint32_t>(header_2->type)), 2u /* = ecaludp::v5::message_type_uint32t::msg_type_fragment */);
  ASSERT_EQ(le32toh(header_2->id), common_id);
  ASSERT_EQ(le32toh(header_2->num), 0);
  ASSERT_EQ(le32toh(header_2->len), 100 - sizeof(ecaludp::v5::Header));

  // Check the header of the last fragment
  auto* header_3 = reinterpret_cast<ecaludp::v5::Header*>(binary_buffer_3->data());
  ASSERT_EQ(header_3->version, 5);
  ASSERT_EQ(le32toh(static_cast<uint32_t>(header_3->type)), 2u /* = ecaludp::v5::message_type_uint32t::msg_type_fragment */);
  ASSERT_EQ(le32toh(header_3->id),  common_id);
  ASSERT_EQ(le32toh(header_3->num), 1);
  ASSERT_EQ(le32toh(header_3->len), message_size - (100 - sizeof(ecaludp::v5::Header)));

  // Create a reassembly object
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassemble the first datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_1, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the second datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_2, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the third datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_3, sender_endpoint, error);

    // The reassembly must have succeeded
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);

    // The message must not be nullptr
    ASSERT_NE(message, nullptr);

    // The message must have the same size as the original buffer
    ASSERT_EQ(message->size(), message_size);

    // The message must contain the same data as the original buffer
    ASSERT_EQ(std::memcmp(message->data(), message_to_send.data(), message_size), 0);
  }
}

// Check the defragmentation of a long message that is larger than the MTU and arrives out of order
TEST(FragmentationV5Test, OutOfOrderFragments)
{
  // Create a longer string
  auto message_to_send = std::string("In the beginning the Universe was created. This had made many people very angry and has been widely regarded as a bad move.");
  const size_t message_size = message_to_send.size();

  // Create an asio buffer from the string
  asio::const_buffer message_to_send_buffer = asio::buffer(message_to_send);

  // Let the datagram builder create fragments for the buffer with a max datagram size of 100 bytes (including header)
  auto datagram_list = ecaludp::v5::create_datagram_list({message_to_send_buffer}, 100, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 3 entries: 1 fragment info and 2 fragments
  ASSERT_EQ(datagram_list.size(), 3);

  // The size of the datagram list is the size of the buffer plus the size of the header
  ASSERT_EQ(datagram_list[0].size(), sizeof(ecaludp::v5::Header)); // This is the fragment info
  ASSERT_EQ(datagram_list[1].size(), 100);                         // This is the entire full datagram
  ASSERT_EQ(datagram_list[2].size(), sizeof(ecaludp::v5::Header) + message_size - (100 - sizeof(ecaludp::v5::Header))); // This is the last fragment

  // Copy the datagram list to a couple of binary buffers
  auto binary_buffer_1 = to_binary_buffer(datagram_list[0]);
  auto binary_buffer_2 = to_binary_buffer(datagram_list[1]);
  auto binary_buffer_3 = to_binary_buffer(datagram_list[2]);

  // create a reassembly object
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassemble the third datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_3, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the first datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_1, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the second datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_2, sender_endpoint, error);

    // The reassembly must have succeeded
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);

    // The message must not be nullptr
    ASSERT_NE(message, nullptr);

    // The message must have the same size as the original buffer
    ASSERT_EQ(message->size(), message_size);

    // The message must contain the same data as the original buffer
    ASSERT_EQ(std::memcmp(message->data(), message_to_send.data(), message_size), 0);
  }
}

// Check the handling of a 1-fragment-message (i.e. a message that is small enough to fit into a single datagram, but is still fragmented)
TEST(FragmentationV5Test, SingleFragmentFragmentation)
{
  // Create a Hello World string
  std::string hello_world = "Hello World!";

  // Create an asio buffer from the string
  asio::const_buffer hello_world_buffer = asio::buffer(hello_world);

  // Let the datagram builder create fragments for the buffer with a max datagram size of 100 bytes (including header)
  auto datagram_list = ecaludp::v5::create_fragmented_datagram_list({hello_world_buffer}, 100, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 2 entries: 1 fragment info and 1 fragment
  ASSERT_EQ(datagram_list.size(), 2);

  // Check the size of the datagrams
  ASSERT_EQ(datagram_list[0].size(), sizeof(ecaludp::v5::Header)); // This is the fragment info
  ASSERT_EQ(datagram_list[1].size(), sizeof(ecaludp::v5::Header) + hello_world.size()); // This is the fragment

  // Copy the datagram list to a couple of binary buffers
  auto binary_buffer_1 = to_binary_buffer(datagram_list[0]);
  auto binary_buffer_2 = to_binary_buffer(datagram_list[1]);

  // Check the header of the fragment info
  auto* header_1 = reinterpret_cast<ecaludp::v5::Header*>(binary_buffer_1->data());
  auto common_id = le32toh(header_1->id);
  ASSERT_EQ(header_1->version, 5);
  ASSERT_EQ(le32toh(static_cast<uint32_t>(header_1->type)), 1u /* = ecaludp::v5::message_type_uint32t::msg_type_fragment_info */);
  ASSERT_EQ(le32toh(header_1->num), 1);
  ASSERT_EQ(le32toh(header_1->id), common_id);
  ASSERT_EQ(le32toh(header_1->len), hello_world.size());

  // Check the header of the first fragment
  auto* header_2 = reinterpret_cast<ecaludp::v5::Header*>(binary_buffer_2->data());
  ASSERT_EQ(header_2->version, 5);
  ASSERT_EQ(le32toh(static_cast<uint32_t>(header_2->type)), 2u /* = ecaludp::v5::message_type_uint32t::msg_type_fragment */);
  ASSERT_EQ(le32toh(header_2->id), common_id);
  ASSERT_EQ(le32toh(header_2->num), 0);
  ASSERT_EQ(le32toh(header_2->len), hello_world.size());

  // Create a reassembly object
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassemble the first datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_1, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the second datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_2, sender_endpoint, error);

    // The reassembly must have succeeded
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);

    // The message must not be nullptr
    ASSERT_NE(message, nullptr);

    // The message must have the same size as the original buffer
    ASSERT_EQ(message->size(), hello_world.size());

    // The message must contain the same data as the original buffer
    ASSERT_EQ(std::memcmp(message->data(), hello_world.data(), hello_world.size()), 0);
  }
}

// Check "Fragmentation" and "Defragmentation" of a 0-byte message
TEST(FragmentationV5Test, ZeroByteMessage)
{
  // Create a 0-byte string
  std::string zero_byte_string;

  // Create an asio buffer from the string
  asio::const_buffer zero_byte_buffer = asio::buffer(zero_byte_string);

  // Let the datagram builder create fragments for the buffer with a max datagram size of 1000 bytes (including header)
  auto datagram_list = ecaludp::v5::create_datagram_list({zero_byte_buffer}, 1000, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 1 entry
  ASSERT_EQ(datagram_list.size(), 1);

  // The size of the datagram list is the size of the buffer plus the size of the header
  ASSERT_EQ(datagram_list.front().size(), sizeof(ecaludp::v5::Header));

  // Copy the datagram list to a binary buffer
  auto binary_buffer = to_binary_buffer(datagram_list.front());

  // Check the header
  auto* header = reinterpret_cast<ecaludp::v5::Header*>(binary_buffer->data());
  ASSERT_EQ(header->version, 5);
  ASSERT_EQ(le32toh(static_cast<uint32_t>(header->type)), 3u /* = ecaludp::v5::message_type_uint32t::msg_type_non_fragmented_message */);
  ASSERT_EQ(le32toh(header->id), -1);
  ASSERT_EQ(le32toh(header->num), 1);
  ASSERT_EQ(le32toh(header->len), 0);

  // Create a reassembly object
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassebly the datagram
  ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
  auto message = reassembly.handle_datagram(binary_buffer, sender_endpoint, error);

  // The reassembly must have succeeded
  ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);

  // The message must not be nullptr
  ASSERT_NE(message, nullptr);

  // The message must have the same size as the original buffer
  ASSERT_EQ(message->size(), 0);
}

// Check Fragmentation and defragmentation of a muli-buffer-message
TEST(FragmentationV5Test, MultiBufferFragmentation)
{
  auto message_to_send_1 = std::make_shared<std::string>("In the beginning the Universe was created.");
  auto message_to_send_2 = std::make_shared<std::string>(" ");
  auto message_to_send_3 = std::make_shared<std::string>("This had made many people very angry and has been widely regarded as a bad move.");

  // Create an asio buffer from the string
  asio::const_buffer message_to_send_buffer_1 = asio::buffer(*message_to_send_1);
  asio::const_buffer message_to_send_buffer_2 = asio::buffer(*message_to_send_2);
  asio::const_buffer message_to_send_buffer_3 = asio::buffer(*message_to_send_3);

  // create the entire message for later
  std::string entire_message = *message_to_send_1 + *message_to_send_2 + *message_to_send_3;

  // Let the datagram builder create fragments for the buffer with a max datagram size of 100 bytes (including header)
  auto datagram_list = ecaludp::v5::create_datagram_list({message_to_send_buffer_1, message_to_send_buffer_2, message_to_send_buffer_3}, 70, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 4 entries: 1 fragment info and 3 fragments
  ASSERT_EQ(datagram_list.size(), 4);

  // The size of the datagram list is the size of the buffer plus the size of the header
  ASSERT_EQ(datagram_list[0].size(), sizeof(ecaludp::v5::Header)); // This is the fragment info
  ASSERT_EQ(datagram_list[1].size(), 70);                          // This is the first fragment
  ASSERT_EQ(datagram_list[2].size(), 70);                          // This is the second fragment

  int payload_per_datagram = 70 - sizeof(ecaludp::v5::Header);
  ASSERT_EQ(datagram_list[3].size(), sizeof(ecaludp::v5::Header) + entire_message.size() - (2 * payload_per_datagram)); // This is the last fragment

  // Copy the datagram list to a couple of binary buffers
  auto binary_buffer_1 = to_binary_buffer(datagram_list[0]);
  auto binary_buffer_2 = to_binary_buffer(datagram_list[1]);
  auto binary_buffer_3 = to_binary_buffer(datagram_list[2]);
  auto binary_buffer_4 = to_binary_buffer(datagram_list[3]);

  // Create the reassembly
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassemble the first datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_1, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the second datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_2, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the third datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_3, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the fourth datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_4, sender_endpoint, error);

    // The reassembly must have succeeded
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);

    // The message must not be nullptr
    ASSERT_NE(message, nullptr);

    // The message must have the same size as the original buffer
    ASSERT_EQ(message->size(), entire_message.size());

    // The message must contain the same data as the original buffer
    ASSERT_EQ(std::memcmp(message->data(), entire_message.data(), entire_message.size()), 0);
  }
}

// Test if a tailing zero-buffer will create a new fragment (it shouldn't)
TEST(FragmentationV5Test, MultiBufferWithTailingZeroBuffer)
{
  auto message_to_send_1 = std::make_shared<std::string>("Hello World!");
  auto message_to_send_2 = std::make_shared<std::string>("");
  auto message_to_send_3 = std::make_shared<std::string>("");

  // Create an asio buffer from the string
  asio::const_buffer message_to_send_buffer_1 = asio::buffer(*message_to_send_1);
  asio::const_buffer message_to_send_buffer_2 = asio::buffer(*message_to_send_2);
  asio::const_buffer message_to_send_buffer_3 = asio::buffer(*message_to_send_3);

  // Compute the UDP Datagram size. The goal is to fill exactly 2 datagrams with the "Hello World" message
  ASSERT_EQ(0, message_to_send_1->size() % 2); // Quickly check that the message has a size dividable by 2
  size_t max_datagram_size = sizeof(ecaludp::v5::Header) + (message_to_send_1->size() / 2);

  // Let the datagram builder create fragments for the buffer with the computed max datagram size
  auto datagram_list = ecaludp::v5::create_datagram_list({message_to_send_buffer_1, message_to_send_buffer_2}, max_datagram_size, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 3 entries: 1 fragment info and 2 fragments. The empty message must have disappeared
  ASSERT_EQ(datagram_list.size(), 3);

  // The size of the datagram list is the size of the buffer plus the size of the header
  ASSERT_EQ(datagram_list[0].size(), sizeof(ecaludp::v5::Header)); // This is the fragment info
  ASSERT_EQ(datagram_list[1].size(), sizeof(ecaludp::v5::Header) + message_to_send_1->size() / 2); // This is the first fragment
  ASSERT_EQ(datagram_list[2].size(), sizeof(ecaludp::v5::Header) + message_to_send_1->size() / 2); // This is the second fragment

  // Copy the datagram list to a couple of binary buffers
  auto binary_buffer_1 = to_binary_buffer(datagram_list[0]);
  auto binary_buffer_2 = to_binary_buffer(datagram_list[1]);
  auto binary_buffer_3 = to_binary_buffer(datagram_list[2]);

  // Create the reassembly
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassemble all datagrams
  {
    ecaludp::Error error (ecaludp::Error::GENERIC_ERROR);

    reassembly.handle_datagram(binary_buffer_1, sender_endpoint, error);
    reassembly.handle_datagram(binary_buffer_2, sender_endpoint, error);
    auto reassembled_message = reassembly.handle_datagram(binary_buffer_3, sender_endpoint, error);

    // The reassembly must have succeeded
    ASSERT_EQ(error, ecaludp::Error::OK);

    // The message must not be nullptr
    ASSERT_NE(reassembled_message, nullptr);

    // The message must have the same size as the original buffer
    ASSERT_EQ(reassembled_message->size(), message_to_send_1->size());

    // The message must contain the same data as the original buffer
    ASSERT_EQ(std::memcmp(reassembled_message->data(), message_to_send_1->data(), message_to_send_1->size()), 0);
  }
}

// Test whether "too old" packages are removed from the reassembly
TEST(FragmentationV5Test, Cleanup)
{
  // Create 2 messages that are the same size
  auto message_1 = std::make_shared<std::string>("In the beginning the Universe was created.");
  auto message_2 = std::make_shared<std::string>("Hello World!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");

  // Create an asio buffer from the strings
  asio::const_buffer message_1_buffer = asio::buffer(*message_1);
  asio::const_buffer message_2_buffer = asio::buffer(*message_2);

  // Let the datagram builder create fragments for the buffer with a max datagram size of 40 bytes (including header)
  auto datagram_list_1 = ecaludp::v5::create_datagram_list({message_1_buffer}, 60, {'E', 'C', 'A', 'L'});
  auto datagram_list_2 = ecaludp::v5::create_datagram_list({message_2_buffer}, 60, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 3 entries: 1 fragment info and 2 fragments
  ASSERT_EQ(datagram_list_1.size(), 3);
  ASSERT_EQ(datagram_list_2.size(), 3);

  // Parse the datagram lists to binary buffers
  auto binary_buffer_1_1 = to_binary_buffer(datagram_list_1[0]);
  auto binary_buffer_1_2 = to_binary_buffer(datagram_list_1[1]);
  auto binary_buffer_1_3 = to_binary_buffer(datagram_list_1[2]);

  auto binary_buffer_2_1 = to_binary_buffer(datagram_list_2[0]);
  auto binary_buffer_2_2 = to_binary_buffer(datagram_list_2[1]);
  auto binary_buffer_2_3 = to_binary_buffer(datagram_list_2[2]);

  // Create a reassembly object
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Reassemble the first datagram of the first message
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_1_1, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the second datagram of the first message
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message_1 = reassembly.handle_datagram(binary_buffer_1_2, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message_1, nullptr);
  }

  // Safe the current time
  auto current_time = std::chrono::steady_clock::now();

  // sleep 1 ms
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  // Reassemble the first datagram of the second message
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_2_1, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the second datagram of the second message
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message_2 = reassembly.handle_datagram(binary_buffer_2_2, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message_2, nullptr);
  }

  // Cleanup old packages (this should hit the first message only)
  reassembly.remove_old_packages(current_time);

  // Reassemble the third datagram of the first message. This should fail, as the first message should have been removed
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message_1 = reassembly.handle_datagram(binary_buffer_1_3, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as the first fragments should have been removed
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message_1, nullptr);
  }

  // Reassemble the third datagram of the second message
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message_2 = reassembly.handle_datagram(binary_buffer_2_3, sender_endpoint, error);

    // The reassembly must have succeeded
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);

    // The message must not be nullptr
    ASSERT_NE(message_2, nullptr);

    // The message must have the same size as the original buffer
    ASSERT_EQ(message_2->size(), message_2->size());

    // The message must contain the same data as the original buffer
    ASSERT_EQ(std::memcmp(message_2->data(), message_2->data(), message_2->size()), 0);
  }
}

TEST(FragmentationV5Test, FaultyFragmentedMessages)
{
  // Create a longer string
  auto message_to_send = std::string("In the beginning the Universe was created. This had made many people very angry and has been widely regarded as a bad move.");
  const size_t message_size = message_to_send.size();

  // Create an asio buffer from the string
  asio::const_buffer message_to_send_buffer = asio::buffer(message_to_send);

  // Let the datagram builder create fragments for the buffer with a max datagram size of 100 bytes (including header)
  auto datagram_list = ecaludp::v5::create_datagram_list({message_to_send_buffer}, 100, {'E', 'C', 'A', 'L'});

  // The datagram list must have exactly 3 entries: 1 fragment info and 2 fragments
  ASSERT_EQ(datagram_list.size(), 3);

  // Convert to binary buffers
  auto binary_buffer_1 = to_binary_buffer(datagram_list[0]);
  auto binary_buffer_2 = to_binary_buffer(datagram_list[1]);
  auto binary_buffer_3 = to_binary_buffer(datagram_list[2]);

  // Create the reassembly
  ecaludp::v5::Reassembly reassembly;

  // Create a fake sender endpoint as shared_ptr
  auto sender_endpoint = std::make_shared<asio::ip::udp::endpoint>();
  sender_endpoint->address(asio::ip::address::from_string("127.0.0.1"));
  sender_endpoint->port(1234);

  // Add some way too small fake datagram to the reassembly. This fails, as the datagram cannot even fit a header
  {
    auto fake_datagram = std::make_shared<ecaludp::RawMemory>(8);
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(fake_datagram, sender_endpoint, error);

    // The reassembly must have failed, as the datagram is too small
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the first datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_1, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }

  // Reassemble the first datagram again (-> duplicate datagram)
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_1, sender_endpoint, error);

    // The reassembly must have failed, as the message is a duplicate
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::DUPLICATE_DATAGRAM);
    ASSERT_EQ(message, nullptr);
  }

  // Copy the second datagram to a new buffer and change the size information to something large, so the datagram doesn't even contain enough data
  {
    auto faulty_binary_buffer_2 = std::make_shared<ecaludp::RawMemory>(*binary_buffer_2);
    auto* header = reinterpret_cast<ecaludp::v5::Header*>(faulty_binary_buffer_2->data());
    header->len = htole32(1000);

    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(faulty_binary_buffer_2, sender_endpoint, error);

    // The reassembly must have failed, as the message is faulty
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM);
    ASSERT_EQ(message, nullptr);
  }

  // Copy the second datagram to a new buffer and modify the type to something invalid
  {
    auto faulty_binary_buffer_2 = std::make_shared<ecaludp::RawMemory>(*binary_buffer_2);
    auto* header = reinterpret_cast<ecaludp::v5::Header*>(faulty_binary_buffer_2->data());
    header->type = static_cast<ecaludp::v5::message_type_uint32t>(1000);

    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(faulty_binary_buffer_2, sender_endpoint, error);

    // The reassembly must have failed, as the message is faulty
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM);
    ASSERT_EQ(message, nullptr);
  }

  // Actually add fragment 2
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message_2 = reassembly.handle_datagram(binary_buffer_2, sender_endpoint, error);

    // The reassembly must have succeeded, but the message must be nullptr, as it is not yet complete
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message_2, nullptr);
  }

  // Add fragment 2 again, this time it is a duplicate datagram
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message_2 = reassembly.handle_datagram(binary_buffer_2, sender_endpoint, error);

    // The reassembly must have failed, as the message is a duplicate
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::DUPLICATE_DATAGRAM);
    ASSERT_EQ(message_2, nullptr);
  }

  // Copy fragment 3, but change the num to fragment 4 that DOES NOT EXIST (-> i.e. num = 2).
  {
    auto faulty_binary_buffer_3 = std::make_shared<ecaludp::RawMemory>(*binary_buffer_3);
    auto* header = reinterpret_cast<ecaludp::v5::Header*>(faulty_binary_buffer_3->data());
    header->num = htole32(2);

    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(faulty_binary_buffer_3, sender_endpoint, error);

    // The reassembly must have failed, as the message is faulty (we know that it does not fit in the list)
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::MALFORMED_DATAGRAM);
    ASSERT_EQ(message, nullptr);
  }

  // Copy fragment 2, but change the num to fragment 3 (-> i.e. num = 1). This will cause the entire message to corrupt.
  {
    auto faulty_binary_buffer_2 = std::make_shared<ecaludp::RawMemory>(*binary_buffer_2);
    auto* header = reinterpret_cast<ecaludp::v5::Header*>(faulty_binary_buffer_2->data());
    header->num = htole32(1);

    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(faulty_binary_buffer_2, sender_endpoint, error);

    // The reassembly must have failed, as the message is faulty
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::MALFORMED_REASSEMBLED_MESSAGE);
    ASSERT_EQ(message, nullptr);
  }

  // Add the actual fragment 3 now. Unfortunatelly, the last faulty fragment 2 caused the reassembly to drop the message, so this will not return the reassebled message, either
  {
    ecaludp::Error error = ecaludp::Error::ErrorCode::GENERIC_ERROR;
    auto message = reassembly.handle_datagram(binary_buffer_3, sender_endpoint, error);

    // The reassembly succeeds, but the message is nullptr, as the message was dropped as being corrupt, before.
    ASSERT_EQ(error, ecaludp::Error::ErrorCode::OK);
    ASSERT_EQ(message, nullptr);
  }
}

// TODO: Test adding messages from more than 1 sender to the reassembly