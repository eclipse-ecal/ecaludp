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

#include <iostream>

#include <asio.hpp>
#include <ecaludp/socket_npcap.h>

#include <thread>

int main()
{
  // Create a socket
  ecaludp::SocketNpcap socket(io_context, {'E', 'C', 'A', 'L'});

  // Bind the socket
  bool success = socket.bind(asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 14000));
  
  // Print whether binding was successfull. If the npcap driver is not installed, this will fail, but not crash.
  std::cout << "Bind success: " << (success ? "true" : "false") << std::endl;
  
  return 0;
}
