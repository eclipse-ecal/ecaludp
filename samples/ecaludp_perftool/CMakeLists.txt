################################################################################
# Copyright (c) 2024 Continental Corporation
# 
# This program and the accompanying materials are made available under the
# terms of the Apache License, Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0.
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
# 
# SPDX-License-Identifier: Apache-2.0
################################################################################

cmake_minimum_required(VERSION 3.13)

project(ecaludp_perftool)

find_package(Threads REQUIRED)
find_package(ecaludp REQUIRED)

set(sources
  src/main.cpp
  src/receiver.cpp
  src/receiver.h
  src/receiver_async.cpp
  src/receiver_async.h
  src/receiver_parameters.h
  src/receiver_sync.cpp
  src/receiver_sync.h
  src/sender.cpp
  src/sender.h
  src/sender_async.cpp
  src/sender_async.h
  src/sender_parameters.h
  src/sender_sync.cpp
  src/sender_sync.h
  src/socket_builder_asio.cpp
  src/socket_builder_asio.h
)
if (${ECALUDP_ENABLE_NPCAP})
  list (APPEND sources
	src/receiver_npcap_async.cpp
	src/receiver_npcap_async.h
	src/receiver_npcap_sync.cpp
	src/receiver_npcap_sync.h
    src/socket_builder_npcap.cpp
    src/socket_builder_npcap.h
  )
endif()

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    ecaludp::ecaludp
    Threads::Threads)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${sources}
)
