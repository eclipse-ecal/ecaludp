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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <asio.hpp> // IWYU pragma: keep

#include "receiver.h"
#include "receiver_async.h"
#include "receiver_parameters.h"
#include "receiver_sync.h"
#include "sender.h"
#include "sender_async.h"
#include "sender_parameters.h"
#include "sender_sync.h"

#if ECALUDP_UDPCAP_ENABLED
  #include "receiver_npcap_sync.h"
  #include "receiver_npcap_async.h"
#endif // ECALUDP_UDPCAP_ENABLED

enum class Implementation
{
  NONE,
  SEND,
  SENDASYNC,
  RECEIVE,
  RECEIVEASYNC,
  RECEIVENPCAP,
  RECEIVENPCAPASYNC
};

void printUsage(const std::string& arg0)
{
  std::cout << "Usage:" << std::endl;
  std::cout << "  " << arg0 << " <IMPLEMENTATION> [PARAMETERS]" << std::endl;
  std::cout << "With IMPLEMENTATION one of:" << std::endl;
  std::cout << "  send                Asio-based sender using send_to in a while-loop" << std::endl;
  std::cout << "  sendasync           Asio-based sender using async_send_to" << std::endl;
  std::cout << "  receive             Asio-based receiver using receive_from in a while-loop" << std::endl;
  std::cout << "  receiveasync        Asio-based receiver using async_receive_from" << std::endl;
  std::cout << "  receivenpcap        Npcap-based receiver using receive_from in a while-loop" << std::endl;
  std::cout << "  receivenpcapasync   Npcap-based receiver using async_receive_from" << std::endl;
  std::cout << std::endl;
  std::cout << "Options:" << std::endl;
  std::cout << "  -h, --help  Show this help message and exit" << std::endl;
  std::cout << std::endl;
  std::cout << "      --ip <IP> IP address to send to / receive from" << std::endl;
  std::cout << "      --port <PORT> Port to send to / receive from" << std::endl;
  std::cout << "  -s, --size <SIZE> Message size to send" << std::endl;
  std::cout << "  -m, --max-udp-datagram-size <SIZE> Maximum UDP datagram size" << std::endl;
  std::cout << "      --buffer-size <SIZE> Buffer size for sending & receiving messages" << std::endl;
  std::cout << std::endl;
}

int main(int argc, char* argv[])
{
  Implementation implementation = Implementation::NONE;

  ReceiverParameters receiver_parameters;
  SenderParameters   sender_parameters;

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

  // Check for implementation
  {
    if (args[1] == "send")
    {
      implementation = Implementation::SEND;
    }
    else if (args[1] == "sendasync")
    {
      implementation = Implementation::SENDASYNC;
    }
    else if (args[1] == "receive")
    {
      implementation = Implementation::RECEIVE;
    }
    else if (args[1] == "receiveasync")
    {
      implementation = Implementation::RECEIVEASYNC;
    }
    else if (args[1] == "receivenpcap")
    {
      implementation = Implementation::RECEIVENPCAP;
    }
    else if (args[1] == "receivenpcapasync")
    {
      implementation = Implementation::RECEIVENPCAPASYNC;
    }
    else
    {
      printUsage(args[0]);
      return 1;
    }
  }

  // Check for --ip
  {
    auto it = std::find(args.begin(), args.end(), "--ip");
    if (it != args.end())
    {
      if (it + 1 == args.end())
      {
        std::cerr << "Error: --ip requires an argument" << std::endl;
        return 1;
      }
      sender_parameters.ip   = *(it + 1);
      receiver_parameters.ip = *(it + 1);
    }
  }

  // Check for --port
  {
    auto it = std::find(args.begin(), args.end(), "--port");
    if (it != args.end())
    {
      if (it + 1 == args.end())
      {
        std::cerr << "Error: --port requires an argument" << std::endl;
        return 1;
      }

      unsigned long port {0};
      try
      {
        port = std::stoul(*(it + 1));
      }
      catch (const std::exception& e)
      {
        std::cerr << "Error: --port requires a numeric argument: " << e.what() << std::endl;
        return 1;
      }

      // Check numeric limits and print error if out of range
      if (port > std::numeric_limits<uint16_t>::max())
      {
        std::cerr << "Error: --port out of range" << std::endl;
        return 1;
      }

      sender_parameters.port   = static_cast<uint16_t>(port);
      receiver_parameters.port = static_cast<uint16_t>(port);
    }
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

      try
      {
        sender_parameters.message_size = std::stoul(*(it + 1));
      }
      catch (const std::exception& e)
      {
        std::cerr << "Error: -s / --size requires a numeric argument: " << e.what() << std::endl;
        return 1;
      }
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

      try
      {
        sender_parameters.max_udp_datagram_size = std::stoul(*(it + 1));
      }
      catch (const std::exception& e)
      {
        std::cerr << "Error: -m / --max-udp-datagram-size requires a numeric argument: " << e.what() << std::endl;
        return 1;
      }
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

      unsigned long buffer_size {0};
      try
      {
          buffer_size = std::stoul(*(it + 1));
      }
      catch (const std::exception& e)
      {
          std::cerr << "Error: --buffer-size requires a numeric argument: " << e.what() << std::endl;
          return 1;
      }
      sender_parameters.buffer_size   = buffer_size;
      receiver_parameters.buffer_size = buffer_size;
    }
  }


  // Run the selected implementation
  std::shared_ptr<Sender>   sender;
  std::shared_ptr<Receiver> receiver;

  switch (implementation)
  {
  case Implementation::NONE:
    break;
  case Implementation::SEND:
    sender = std::make_shared<SenderSync>(sender_parameters);
    break;
  case Implementation::SENDASYNC:
    sender = std::make_shared<SenderAsync>(sender_parameters);
    break;
  case Implementation::RECEIVE:
    receiver = std::make_shared<ReceiverSync>(receiver_parameters);
    break;
  case Implementation::RECEIVEASYNC:
    receiver = std::make_shared<ReceiverAsync>(receiver_parameters);
    break;
  case Implementation::RECEIVENPCAP:
#if ECALUDP_UDPCAP_ENABLED
    receiver = std::make_shared<ReceiverNpcapSync>(receiver_parameters);
    break;
#else
    std::cerr << "Error: Npcap-based receiver not enabled" << std::endl;
    return 1;
#endif // ECALUDP_UDPCAP_ENABLED
  case Implementation::RECEIVENPCAPASYNC:
#if ECALUDP_UDPCAP_ENABLED
    receiver = std::make_shared<ReceiverNpcapAsync>(receiver_parameters);
    break;
#else
    std::cerr << "Error: Npcap-based receiver not enabled" << std::endl;
    return 1;
#endif // ECALUDP_UDPCAP_ENABLED
  default:
    break;
  }

  if (sender)
  {
    sender->start();
  }
  else if (receiver)
  {
    receiver->start();
  }

  while(true)
    std::this_thread::sleep_for(std::chrono::seconds(1));

  return 0;
}
