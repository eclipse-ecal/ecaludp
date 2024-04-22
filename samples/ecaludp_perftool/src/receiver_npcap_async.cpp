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

#include "receiver_npcap_async.h"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>

#include <asio.hpp>

#include "ecaludp/socket.h"
#include "receiver.h"
#include "receiver_parameters.h"
#include "socket_builder_npcap.h"

ReceiverNpcapAsync::ReceiverNpcapAsync(const ReceiverParameters& parameters)
  : Receiver(parameters)
{
  std::cout << "Receiver implementation: Asynchronous NPCAP\n";
}

ReceiverNpcapAsync::~ReceiverNpcapAsync()
{
  if (socket_)
  {
    socket_->close();
  }
}

void ReceiverNpcapAsync::start()
{
  try
  {
     socket_ = SocketBuilderNpcap::CreateReceiveSocket(parameters_);
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error creating socket: " << e.what()<< '\n';
    std::exit(1);
  }

  receive_message();
}

void ReceiverNpcapAsync::receive_message()
{
  auto endpoint = std::make_shared<asio::ip::udp::endpoint>();

  socket_->async_receive_from(*endpoint,
                              [this, endpoint](const std::shared_ptr<ecaludp::OwningBuffer>& message, const ecaludp::Error& error)
                              {
                                if (error)
                                {
                                  std::cerr << "Error sending: " << error.ToString()<< '\n';
                                  socket_->close();
                                  return;
                                }

                                {
                                  const std::lock_guard<std::mutex> lock(statistics_mutex_);

                                  bytes_payload_     += message->size();
                                  messages_received_ ++;
                                }

                                receive_message();
                              });
}
