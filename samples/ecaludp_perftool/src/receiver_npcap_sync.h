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

class ReceiverNpcapSync : public Receiver
{
  public:
    ReceiverNpcapSync(const ReceiverParameters& parameters);
    ~ReceiverNpcapSync() override;

    // disable copy and move
    ReceiverNpcapSync(const ReceiverNpcapSync&) = delete;
    ReceiverNpcapSync(ReceiverNpcapSync&&) = delete;
    ReceiverNpcapSync& operator=(const ReceiverNpcapSync&) = delete;
    ReceiverNpcapSync& operator=(ReceiverNpcapSync&&) = delete;

    void start() override;

  private:
    void receive_loop();

  private:
    std::unique_ptr<std::thread> receive_thread_;
};
