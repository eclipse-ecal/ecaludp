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
#include <mutex>
#include <condition_variable>

class Receiver
{
public:
  Receiver(int buffer_size);
  virtual ~Receiver();

  virtual void start() = 0;

private:
  void print_statistics();

///////////////////////////////////////////////////////////
// Member variables
///////////////////////////////////////////////////////////

protected:
  int                           buffer_size_;

  bool                          is_stopped_         {false};
  mutable std::mutex            statistics_mutex_;
  std::condition_variable       cv_;

  long long bytes_payload_    {0};
  long long messages_received_{0};

private:
  std::unique_ptr<std::thread>  statistics_thread_;
};
