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

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <new>

namespace ecaludp
{
  /**
   * @brief A class to manage raw memory
   * 
   * The RawMemory class is an RAII class to manage raw memory. The access to
   * the memory is handled as a pointer to the raw memory and the size of the
   * currently used memory. The capacity of the raw memory can be increased
   * manually using the reserve() method or automatically using the resize()
   * method, when the capacity is too low.
   * 
   * This RawMemory class will never modify the underlying memory, even on
   * resizing. Thus, the memory is not initialized and may contain garbage data.
   * The memory is only re-allocated when the capacity is increased.
   * 
   * The reason for this is the assumption that after resizing the memory is
   * either written to or downsized again before reading. Thus, non initializing
   * the memory is a runtime optimization.
   */
  class RawMemory
  {
  public:
    /**
     * @brief Construct a new RawMemory object with no memory allocated
     * 
     * No memory is allocated. The data pointer is set to nullptr and the size
     * and capacity will be 0. Use the reserve() or resize() methods to allocate
     * memory.
     */
	RawMemory()
	  : data_    (nullptr)
      , capacity_(0)
      , size_    (0)
    {}

    /**
     * @brief Construct a new RawMemory object with the given size
     * 
     * The memory is allocated with the given size exactly. There is no
     * overprovisioning like other STL containers usually do. If it is intended
     * to later increase the size of this memory chunk, it is recommended to
     * create with a greater size and then downsize to the currently needed
     * size. The memory is not initialized and may contain garbage data.
     * 
     * @param size The size of the memory to allocate
     */
	RawMemory(size_t size)
      : data_    (malloc(size))
      , capacity_(size)
      , size_    (size)
    {
      // Throw exception if we could not allocate memory
      if ((capacity_ > 0) && (data_ == nullptr))
      {
        throw std::bad_alloc();
      }
    }

    // Copy constructor
	RawMemory(const RawMemory& other)
	  : data_    (malloc(other.capacity_))
      , capacity_(other.capacity_)
      , size_    (other.size_)
  	{
      // Throw exception if we could not allocate memory
      if ((capacity_ > 0) && (data_ == nullptr))
      {
        throw std::bad_alloc();
      }

      if ((data_ != nullptr) && (other.data_ != nullptr))
        memcpy(data_, other.data_, size_);
    }

    // Move constructor
	RawMemory(RawMemory&& other) noexcept
	  : data_    (other.data_)
      , capacity_(other.capacity_)
      , size_    (other.size_)
	{
      other.data_     = nullptr;
      other.capacity_ = 0;
      other.size_     = 0;
    }

    // Copy assignment operator
	RawMemory& operator=(const RawMemory& other)
	{
	  if (this != &other)
	  {
        reserve(other.capacity_);
        resize(other.size_);
        if ((data_ != nullptr) && (other.data_ != nullptr))
          memcpy(data_, other.data_, size_);
      }
      return *this;
    }

    // Move assignment operator
	RawMemory& operator=(RawMemory&& other) noexcept
	{
	  if (this != &other)
	  {
        free(data_);
        
        data_       = other.data_;
        capacity_   = other.capacity_;
        size_       = other.size_;

        other.data_     = nullptr;
        other.capacity_ = 0;
        other.size_     = 0;
      }
      return *this;
    }

    // Destructor
	~RawMemory() noexcept
	{
      free(data_);
	}

    void swap(RawMemory& other) noexcept
    {
      std::swap(data_,     other.data_);
      std::swap(capacity_, other.capacity_);
      std::swap(size_,     other.size_);
    }

    /**
     * @brief Return a pointer to the raw memory
     * 
     * The data is returned as a pointer to the raw memory. Use the size()
     * method to get the size of the currently used memory.
     * 
     * The data pointer will be invalidated if the memory is re-allocated by
     * increasing the capacity byond the current capacity.
     * 
     * @return A pointer to the raw memory
     */
    uint8_t* data() const
    {
      return reinterpret_cast<uint8_t*>(data_);
    }

    /**
     * @brief Return the size of the currently used memory
     * 
     * This is the size of the memory that is currently used. It is not the same
     * as the capacity of the raw memory, which can be obtained using the
     * capacity() method. The size is always less than or equal to the capacity.
     * 
     * @return The size of the currently used memory
     */
    size_t size() const
    {
      return size_;
    }

    /**
     * @brief Return the capacity of the raw memory
     * 
     * This is the size of the raw memory that was allocated. It is not the same
     * as the currently used size, which can be obtained using the size() method.
     * 
     * @return The capacity of the raw memory
     */
    size_t capacity() const
    {
      return capacity_;
    }

    /**
     * @brief Reserve memory making sure it is at least the given size
     * 
     * If the capacity is not large enough, the capacity will be increased. In
     * that case, the internal buffer is re-allocated. Otherwise, the capacity
     * stays the same and no memory is modified.
     * 
     * The actually used size is not modified, but the data may be moved to a
     * different location, as stated above.
     * 
     * @param size The new capacity
     */
    void reserve(size_t size)
    {
      // We only need to do something if the new size is larger than the current capacity
      if (size > capacity_)
      {
        // re-alloc the buffer
        void* new_data = realloc(data_, size);
        if ((size > 0) && (new_data == nullptr))
        {
          throw std::bad_alloc();
        }
        else
        {
          // update data_ pointer and capacity_. We don't need to update size_
          // as we are only reserving memory and not resizing the currently used
          // memory
          data_     = new_data;
          capacity_ = size;
        }
      }
    }

    /**
     * @brief Resize the currently used memory
     * 
     * If the capacity is not large enough, the capacity will be increased. In
     * that case, the internal buffer is re-allocated. Otherwise, the capacity
     * stays the same and only the used size is updated. No memory is modified.
     * 
     * The new memory is not initialized and may contain garbage data.
     * 
     * @param size The new size
     */
    void resize(size_t size)
    {
      // Make sure we have enough capacity
      reserve(size);

      // Set the new size
      size_ = size;
    }

  private:
	void*  data_;             ///< Pointer to the raw memory
    size_t capacity_;         ///< The capacity of the raw memory 
	size_t size_;             ///< The size that is currently used. Must be <= capacity_
  };
}
