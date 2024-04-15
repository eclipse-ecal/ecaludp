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
#include <iostream>
#include <memory>
#include <string>

#include <asio.hpp> // IWYU pragma: keep

#include "sender_sync.h"
#include "receiver_sync.h"

void printUsage(const std::string& arg0)
{
  std::cout << "Usage:" << std::endl;
  std::cout << "  " << arg0 << " send <ip>:<port>" << std::endl;
  std::cout << "or:" << std::endl;
  std::cout << "  " << arg0 << " receive <ip>:<port>" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h, --help  Show this help message and exit" << std::endl;
  std::cout << std::endl;
  std::cout << "  -s, --size <SIZE> Message size to send" << std::endl;
  std::cout << "  -m, --max-udp-datagram-size <SIZE> Maximum UDP datagram size" << std::endl;
  std::cout << "      --buffer-size <SIZE> Buffer size for sending & receiving messages" << std::endl;
  std::cout << std::endl;
}

int main(int argc, char* argv[])
{
  bool sender_mode   = false;
  bool receiver_mode = false;

  size_t message_size          = 0;

  // Default to max udp datagram size for IPv4
  size_t max_udp_datagram_size = 65507;

  int buffer_size = -1;

  // convert argc, argv to vector of strings
  std::vector<std::string> args;
  args.reserve(static_cast<size_t>(argc));
  for (int i = 0; i < argc; ++i)
  {
    args.emplace_back(argv[i]);
  }

  // Check for -h / --help
  if (args.size() < 2
      || std::find(args.begin(), args.end(), "-h") != args.end()
      || std::find(args.begin(), args.end(), "--help") != args.end())
  {
    printUsage(args[0]);
    return 0;
  }

  // Check for -s / --size
  {
    auto it = std::find(args.begin(), args.end(), "-s");
    if (it == args.end())
    {
      it = std::find(args.begin(), args.end(), "--size");
    }

    if (it != args.end())
    {
      if (it + 1 == args.end())
      {
        std::cerr << "Error: -s / --size requires an argument" << std::endl;
        return 1;
      }
      message_size = std::stoul(*(it + 1));
    }
  }

  // Check for -m / --max-udp-datagram-size
  {
    auto it = std::find(args.begin(), args.end(), "-m");
    if (it == args.end())
    {
      it = std::find(args.begin(), args.end(), "--max-udp-datagram-size");
    }

    if (it != args.end())
    {
      if (it + 1 == args.end())
      {
        std::cerr << "Error: -m / --max-udp-datagram-size requires an argument" << std::endl;
        return 1;
      }
      max_udp_datagram_size = std::stoul(*(it + 1));
    }
  }

  // Check for buffer size
  {
    auto it = std::find(args.begin(), args.end(), "--buffer-size");
    if (it != args.end())
    {
      if (it + 1 == args.end())
      {
        std::cerr << "Error: --buffer-size requires an argument" << std::endl;
        return 1;
      }
      buffer_size = std::stoul(*(it + 1));
    }
  }

  // check for mode
  if (args[1] == "send")
  {
    sender_mode = true;
  }
  else if (args[1] == "receive")
  {
    receiver_mode = true;
  }
  else
  {
    printUsage(args[0]);
    return 1;
  }

  if (sender_mode)
  {
    SenderSync sender(message_size, max_udp_datagram_size, buffer_size);
    sender.start();

    while (true)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  else if (receiver_mode)
  {
    ReceiverSync receiver(buffer_size);
    receiver.start();

    while (true)
    {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}
