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

#include <string>

// IWYU pragma: begin_exports
#include <ecaludp/ecaludp_export.h>
// IWYU pragma: end_exports

namespace ecaludp
{
  namespace npcap
  {
    /**
     * @brief Initializes Npcap, if not done already. Must be called before calling any native npcap methods.
     *
     * This method initialized Npcap and must be called at least once before
     * calling any ncap functions.
     * As it always returns true when npcap has been intialized successfully, it
     * can also be used to check whether npcap is available and working properly.
     *
     * If this function returns true, npcap should work.
     *
     * @return True if npcap is working
     */
    ECALUDP_EXPORT bool initialize();
    
    /**
     * @brief Checks whether npcap has been initialized successfully
     * @return true if npcap has been initialized successfully
     */
    ECALUDP_EXPORT bool is_initialized();

    /**
     * @brief Returns a human readible status message.
     *
     * This message is intended to be displayed in a graphical user interface.
     * For terminal based applications it is not needed, as the messages are also
     * printed to stderr.
     *
     * @return The Udpcap status as human-readible text (may be multi-line)
     */
    ECALUDP_EXPORT std::string get_human_readable_error_text();
  }
}