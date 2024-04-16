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

#include <memory>

#include <ecaludp/socket.h>

#include <asio.hpp> // IWYU pragma: keep

#include "sender_parameters.h"
#include "receiver_parameters.h"

namespace SocketBuilderAsio
{
  std::shared_ptr<ecaludp::Socket> CreateSendSocket   (asio::io_context& io_context, const SenderParameters&   parameters);
  std::shared_ptr<ecaludp::Socket> CreateReceiveSocket(asio::io_context& io_context, const ReceiverParameters& parameters);
}
