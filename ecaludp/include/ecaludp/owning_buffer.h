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

#include <cstddef>
#include <memory>

namespace ecaludp
{
  /**
   * @brief A buffer-view class that manages the ownership of the internal memory through a generic std::shared_ptr<void const>
   * 
   * The OwningBuffer class is a buffer that can manage the ownership of its own
   * memory. The memory is managed through a std::shared_ptr<void const>.
   * 
   * - The buffer is represented by pointer + size.
   * 
   * - The user creating this object must make sure that the the pointer and size is valid.
   * 
   * - It is assumed that the pointer points to data that is owned by the shared_ptr.
   * 
   * - The user must make sure that the pointer is valid as long as the OwningBuffer
   *   is used. This probably requires to stop writing to the internal buffer as
   *   soon as the OwningBuffer is created. It especially requires to make sure
   *   the internal data will never be re-allocated.
   */
  class OwningBuffer
  {
  public:
    /**
     * @brief Construct a new OwningBuffer object
     * 
     * The OwningBuffer object is constructed with a pointer to the internal
     * data and the size of the internal data. The owning_container is a shared
     * pointer that must own the internal data.
     * 
     * @param data              A pointer to the internal data
     * @param size              The size of the internal data
     * @param owning_container  A shared pointer that owns the internal data
     */
    OwningBuffer(const void* data, size_t size, const std::shared_ptr<void const>& owning_container)
      : data_            (data)
      , size_            (size)
      , owning_container_(owning_container)
    {}

    /**
     * @brief Returns the pointer to the internal data
     * @return the pointer to the internal data
     */
    const void* data() const
    {
      return data_;
    }

    /**
     * @brief Returns the size of the internal data
     * @return the size of the internal data
     */
    size_t size() const
    {
      return size_;
    }

  private:
    const void*                 data_;                ///< The pointer to the internal data
    size_t                      size_;                ///< The size of the internal data
    std::shared_ptr<void const> owning_container_;    ///< A shared pointer that owns the internal data
  };
}
