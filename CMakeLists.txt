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

include(CMakeDependentOption)

# Project call
include("${CMAKE_CURRENT_LIST_DIR}/ecaludp/version.cmake")
project(ecaludp VERSION ${ECALUDP_VERSION_MAJOR}.${ECALUDP_VERSION_MINOR}.${ECALUDP_VERSION_PATCH})

# Normalize backslashes from Windows paths
file(TO_CMAKE_PATH "${CMAKE_MODULE_PATH}" CMAKE_MODULE_PATH)
file(TO_CMAKE_PATH "${CMAKE_PREFIX_PATH}" CMAKE_PREFIX_PATH)
message(STATUS "Module Path: ${CMAKE_MODULE_PATH}")
message(STATUS "Prefix Path: ${CMAKE_PREFIX_PATH}")

# CMake Options
option(ECALUDP_BUILD_SAMPLES
       "Build project samples."
       ON)
option(ECALUDP_BUILD_TESTS
       "Build the eCAL UDP tests"
       OFF)

option(ECALUDP_USE_BUILTIN_ASIO
        "Use the builtin asio submodule. If set to OFF, asio must be available from somewhere else (e.g. system libs)."
        ON)
option(ECALUDP_USE_BUILTIN_RECYCLE
        "Use the builtin steinwurf::recycle submodule. If set to OFF, recycle must be available from somewhere else (e.g. system libs)."
        ON)
cmake_dependent_option(ECALUDP_USE_BUILTIN_GTEST
        "Use the builtin GoogleTest submodule. Only needed if ECALUDP_BUILD_TESTS is ON. If set to OFF, GoogleTest must be available from somewhere else (e.g. system libs)."
        ON                       # Default value if dependency is met
        "ECALUDP_BUILD_TESTS"    # Dependency
        OFF)                     # Default value if dependency is not met

# Set Debug postfix
set(CMAKE_DEBUG_POSTFIX            d)
set(CMAKE_MINSIZEREL_POSTFIX       minsize)
set(CMAKE_RELWITHDEBINFO_POSTFIX   reldbg)

# Use builtin asio
if (ECALUDP_USE_BUILTIN_ASIO)
    include("${CMAKE_CURRENT_LIST_DIR}/thirdparty/build-asio.cmake")
endif()

# Use builtin recycle
if (ECALUDP_USE_BUILTIN_RECYCLE)
    include("${CMAKE_CURRENT_LIST_DIR}/thirdparty/build-recycle.cmake")
endif()

# Use builtin gtest
if (ECALUDP_USE_BUILTIN_GTEST)
    include("${CMAKE_CURRENT_LIST_DIR}/thirdparty/build-gtest.cmake")
endif()

# For tests we need to make sure that all shared libraries and executables are 
# put into the same directory. Otherwise the tests will fail on windows.
if(ECALUDP_BUILD_TESTS AND BUILD_SHARED_LIBS AND ECALUDP_USE_BUILTIN_GTEST)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()

# Add main ecaludp library
add_subdirectory(ecaludp)

# Add the ecaludp dummy module
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ecaludp-module)

if (ECALUDP_BUILD_SAMPLES)
    add_subdirectory(samples/ecaludp_sample)
endif()

if (ECALUDP_BUILD_TESTS)
    enable_testing()
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/tests/ecaludp_test")

    # Check if ecaludp is a static lib. We can only add the private tests for
    # static libs and object libs, as we need to have access to the private
    # implementation details.
    get_target_property(ecaludp_target_type ecaludp TYPE)
    if ((ecaludp_target_type STREQUAL STATIC_LIBRARY) OR (ecaludp_target_type STREQUAL OBJECT_LIBRARY))
        add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/tests/ecaludp_private_test")
    endif()
endif()

# Make this package available for packing with CPack
include("${CMAKE_CURRENT_LIST_DIR}/cpack_config.cmake")
