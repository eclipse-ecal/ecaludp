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

#include "receiver.h"

#include <memory>
#include <thread>

#include <asio.hpp>

#include <ecaludp/socket_npcap.h>

class ReceiverNpcapAsync : public Receiver
{
  public:
    ReceiverNpcapAsync(const ReceiverParameters& parameters);
    ~ReceiverNpcapAsync() override;

    // disable copy and move
    ReceiverNpcapAsync(const ReceiverNpcapAsync&) = delete;
    ReceiverNpcapAsync(ReceiverNpcapAsync&&) = delete;
    ReceiverNpcapAsync& operator=(const ReceiverNpcapAsync&) = delete;
    ReceiverNpcapAsync& operator=(ReceiverNpcapAsync&&) = delete;

    void start() override;

  private:
    void receive_message();

  private:
    std::shared_ptr<ecaludp::SocketNpcap>   socket_;
};
