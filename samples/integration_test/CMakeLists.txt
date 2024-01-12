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

project(integration_test)

find_package(Threads REQUIRED)
find_package(ecaludp REQUIRED)

set(sources
  src/main.cpp
)

add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Threads::Threads
    ecaludp::ecaludp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${sources}
)
