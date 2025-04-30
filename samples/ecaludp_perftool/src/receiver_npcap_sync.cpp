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

#include "receiver_npcap_sync.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include <asio.hpp>

#include "ecaludp/socket_npcap.h"
#include "receiver.h"
#include "receiver_parameters.h"
#include "socket_builder_npcap.h"

ReceiverNpcapSync::ReceiverNpcapSync(const ReceiverParameters& parameters)
  : Receiver(parameters)
{
  std::cout << "Receiver implementation: Synchronous NPCAP\n";
}

ReceiverNpcapSync::~ReceiverNpcapSync()
{
  if (receive_thread_ && receive_thread_->joinable())
  {
    receive_thread_->join();
  }
}

void ReceiverNpcapSync::start()
{
  receive_thread_ = std::make_unique<std::thread>(&ReceiverNpcapSync::receive_loop, this);
}

void ReceiverNpcapSync::receive_loop()
{
  std::shared_ptr<ecaludp::SocketNpcap> receive_socket;
  try
  {
     receive_socket = SocketBuilderNpcap::CreateReceiveSocket(parameters_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error creating socket: " << e.what()<< '\n';
    std::exit(1);
  }

  asio::ip::udp::endpoint destination(asio::ip::make_address(parameters_.ip), parameters_.port);

  while (true)
  {
    {
      ecaludp::Error error = ecaludp::Error::GENERIC_ERROR;
      auto payload_buffer = receive_socket->receive_from(destination, error);

      if (error)
      {
        std::cerr << "Error receiving message: " << error.ToString()<< '\n';
        break;
      }

      {
        const std::lock_guard<std::mutex> lock(statistics_mutex_);
      
        if (is_stopped_)
          break;

        bytes_payload_ += payload_buffer->size();
        messages_received_ ++;
      }
    }
  }
}
