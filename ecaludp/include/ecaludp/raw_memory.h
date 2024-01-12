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
  class RawMemory
  {
  public:
    // Default constructor
	RawMemory()
	  : data_    (nullptr)
      , capacity_(0)
      , size_    (0)
    {}

    // Constructor
	RawMemory(size_t size, bool overprovisioning = true)
      : data_    (malloc(overprovisioning? size * 2 : size))
      , capacity_(overprovisioning? size * 2 : size)
      , size_    (size)
    {}

    // Copy constructor
	RawMemory(const RawMemory& other)
	  : data_    (malloc(other.capacity_))
      , capacity_(other.capacity_)
      , size_    (other.size_)
  	{
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
        resize(other.capacity_, false);
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
	~RawMemory()
	{
      free(data_);
	}

    void swap(RawMemory& other) noexcept
    {
      std::swap(data_,     other.data_);
      std::swap(capacity_, other.capacity_);
      std::swap(size_,     other.size_);
    }

    uint8_t* data() const
    {
      return reinterpret_cast<uint8_t*>(data_);
    }

    size_t size() const
    {
      return size_;
    }

    void resize(size_t size, bool overprovisioning = true)
    {
      // If the new size is smaller than the current capacity, we can just set the new size
      if (size <= capacity_)
      {
        size_ = size;
        return;
      }

      // If the new size is larger than the current capacity, we need to allocate new memory

      // Overprovisioning means that we allocate twice the size of the requested
      // size. This is useful to not have to reallocate memory too often, if the
      // user resizes it to a slightly bigger size.
      const size_t new_capacity = (overprovisioning ? size * 2 : size);

      // Allocate new memory
      void* new_data = realloc(data_, new_capacity);
      if (new_data == nullptr)
      {
        throw std::bad_alloc(); // TODO: decide if throwing here is the right thing to do. Also decide if we should throw at the other places as well.
      }
      else
      {
        data_     = new_data;
        capacity_ = new_capacity;
        size_     = size;
      }
    }

  private:
	void*  data_;
    size_t capacity_;
	size_t size_;
  };
}
