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

#include <thread>
#include <memory>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <deque>

#include <udpcap/udpcap_socket.h>

#include <ecaludp/error.h>

namespace ecaludp
{
  class AsyncUdpcapSocket
  {
  /////////////////////////////////////////////////////
  // Constructor/Destructor
  /////////////////////////////////////////////////////
  public:
    AsyncUdpcapSocket();
    ~AsyncUdpcapSocket();

    // Disable copy and move
    AsyncUdpcapSocket(const AsyncUdpcapSocket&)            = delete;
    AsyncUdpcapSocket(AsyncUdpcapSocket&&)                 = delete;
    AsyncUdpcapSocket& operator=(const AsyncUdpcapSocket&) = delete;
    AsyncUdpcapSocket& operator=(AsyncUdpcapSocket&&)      = delete;

  /////////////////////////////////////////////////////
  // udpcap forwarded methods
  /////////////////////////////////////////////////////
  public:
    bool isValid                    () const                                   { return udpcap_socket_.isValid(); }
    bool bind                       (const Udpcap::HostAddress& local_address, uint16_t local_port); // This also starts the wait thread for async receive
    bool isBound                    () const                                   { return udpcap_socket_.isBound(); }
    Udpcap::HostAddress localAddress() const                                   { return udpcap_socket_.localAddress(); }
    uint16_t localPort              () const                                   { return udpcap_socket_.localPort(); }
    bool setReceiveBufferSize       (int size)                                 { return udpcap_socket_.setReceiveBufferSize(size); }
    bool joinMulticastGroup         (const Udpcap::HostAddress& group_address) { return udpcap_socket_.joinMulticastGroup(group_address); }
    bool leaveMulticastGroup        (const Udpcap::HostAddress& group_address) { return udpcap_socket_.leaveMulticastGroup(group_address); }
    void setMulticastLoopbackEnabled(bool enabled)                             { udpcap_socket_.setMulticastLoopbackEnabled(enabled); }
    bool isMulticastLoopbackEnabled () const                                   { return udpcap_socket_.isMulticastLoopbackEnabled(); }
    void close                      ();

  /////////////////////////////////////////////////////
  // Receive methods
  /////////////////////////////////////////////////////
  public:
    size_t receiveFrom(char*                 buffer
                      , size_t               max_buffer_size
                      , Udpcap::HostAddress& sender_address
                      , uint16_t&            sender_port
                      , ecaludp::Error&       error);

    void asyncReceiveFrom( char*                buffer
                         , size_t               max_buffer_size
                         , Udpcap::HostAddress& sender_address
                         , uint16_t&            sender_port
                         , const std::function<void(ecaludp::Error&, size_t)>& read_handler);

  private:
    static void toEcaludpError(const Udpcap::Error& udpcap_error, ecaludp::Error& ecaludp_error);

  /////////////////////////////////////////////////////
  // Wait thread function
  /////////////////////////////////////////////////////
  private:
    void waitForData();

  /////////////////////////////////////////////////////
  // Member Variables
  /////////////////////////////////////////////////////
  private:
    struct AsyncReceiveFromParameters
    {
      char*                 buffer_;
      size_t                max_buffer_size_;
      Udpcap::HostAddress*  sender_address_;
      uint16_t*             sender_port_;
      std::function<void(ecaludp::Error&, size_t)> read_handler_;
    };

    Udpcap::UdpcapSocket         udpcap_socket_;

    std::unique_ptr<std::thread> wait_thread_;
    std::mutex                   wait_thread_trigger_mutex_;
    std::condition_variable      wait_thread_trigger_cv_;
    std::deque<AsyncReceiveFromParameters> async_receive_from_parameters_queue_;
  };
}
