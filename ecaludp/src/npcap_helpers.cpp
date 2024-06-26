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

#include <ecaludp/npcap_helpers.h>

#include <string>

#include <udpcap/npcap_helpers.h>

namespace ecaludp
{
  namespace npcap
  {
    bool initialize() { return Udpcap::Initialize(); }
    
    bool is_initialized() { return Udpcap::IsInitialized(); }

    std::string get_human_readable_error_text() { return Udpcap::GetHumanReadibleErrorText(); }
  }
}