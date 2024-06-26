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

#include "sender.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>
#include <utility>

#include "sender_parameters.h"

Sender::Sender(const SenderParameters& parameters)
  : parameters_(parameters)
  , statistics_thread_{std::make_unique<std::thread>([this]() { this->print_statistics(); })}
{
  // Print information for debug purposes
  std::cout << parameters_.to_string();
}

Sender::~Sender()
{
  {
    // Stop statistics thread
    const std::lock_guard<std::mutex> lock(statistics_mutex_);
    is_stopped_ = true;
    cv_.notify_all();
  }

  if (statistics_thread_->joinable())
    statistics_thread_->join();
}

void Sender::print_statistics()
{
  std::chrono::steady_clock::time_point last_statistics_run;

  while(true)
  {
    long long bytes_raw    {0};
    long long bytes_payload{0};
    long long messages_sent{0};


    {
      std::unique_lock<std::mutex> lock(statistics_mutex_);
      cv_.wait_for(lock, std::chrono::seconds(1), [this]() -> bool { return is_stopped_; });

      if (is_stopped_)
        return;

      std::swap(bytes_raw_, bytes_raw);
      std::swap(bytes_payload_, bytes_payload);
      std::swap(messages_sent_, messages_sent);
    }

    auto now = std::chrono::steady_clock::now();

    // Calculate message per seconds -> frequency)
    double frequency = 0.0;
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(now - last_statistics_run).count();
    if (duration > 0)
    {
      frequency = static_cast<double>(messages_sent) / duration;
    }

    {
      std::stringstream ss;
      ss << "cnt: "   << messages_sent;
      ss << " | ";
      //ss << "snt raw: " << bytes_raw << " pyld: " << bytes_payload; // TODO: The async send doesn't return the raw number of bytes sent
      ss << "snt pyld: " << bytes_payload;
      ss << " | ";
      ss << "freq: " << std::fixed << std::setprecision(1) << frequency;

      std::cout << ss.str() << '\n';
    }

    last_statistics_run = now;
  }
}
