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

#include "sender.h"

#include <cstddef>
#include <memory>
#include <thread>

#include <ecaludp/socket.h>

class SenderAsync : public Sender
{
  public:
    SenderAsync(const SenderParameters& parameters);
    ~SenderAsync() override;

    void start() override;

  private:
    void send_message(const std::shared_ptr<const std::string>& message, const asio::ip::udp::endpoint& endpoint);

  private:
    std::unique_ptr<std::thread>     io_context_thread_;
    asio::io_context                 io_context_;
    std::shared_ptr<ecaludp::Socket> socket_;
};
