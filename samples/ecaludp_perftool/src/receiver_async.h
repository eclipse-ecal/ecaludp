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
#include "receiver_parameters.h"

#include <memory>
#include <thread>

#include <asio.hpp>

#include <ecaludp/socket.h>

class ReceiverAsync : public Receiver
{
  public:
    ReceiverAsync(const ReceiverParameters& parameters);
    ~ReceiverAsync() override;

    // disable copy and move
    ReceiverAsync(const ReceiverAsync&) = delete;
    ReceiverAsync(ReceiverAsync&&) = delete;
    ReceiverAsync& operator=(const ReceiverAsync&) = delete;
    ReceiverAsync& operator=(ReceiverAsync&&) = delete;

    void start() override;

  private:
    void receive_message();

  private:
    std::unique_ptr<std::thread>            io_context_thread_;
    asio::io_context                        io_context_;
    std::shared_ptr<ecaludp::Socket>        socket_;
    using work_guard_t = asio::executor_work_guard<asio::io_context::executor_type>;
    std::unique_ptr<work_guard_t> work_;
};
