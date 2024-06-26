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

include("${CMAKE_CURRENT_LIST_DIR}/version.cmake")
project(ecaludp VERSION ${ECALUDP_VERSION_MAJOR}.${ECALUDP_VERSION_MINOR}.${ECALUDP_VERSION_PATCH})

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Disable default export of symbols
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

find_package(asio    REQUIRED)
find_package(recycle REQUIRED)

message(STATUS "ECALUDP_ENABLE_NPCAP: ${ECALUDP_ENABLE_NPCAP}")
if(ECALUDP_ENABLE_NPCAP)
    find_package(udpcap REQUIRED)
endif()

# Include GenerateExportHeader that will create export macros for us
include(GenerateExportHeader)

###############################################
# Normal library sources (non-npcap)
###############################################
set (includes
    include/ecaludp/error.h
    include/ecaludp/owning_buffer.h
    include/ecaludp/raw_memory.h
    include/ecaludp/socket.h
)

set(sources
    src/socket.cpp
    src/protocol/datagram_builder_v5.cpp
    src/protocol/datagram_builder_v5.h
    src/protocol/datagram_description.h
    src/protocol/header_common.h
    src/protocol/header_v5.h
    src/protocol/header_v6.h
    src/protocol/portable_endian.h
    src/protocol/reassembly_v5.cpp
    src/protocol/reassembly_v5.h
)

###############################################
# Sources for npcap enabled build
###############################################
if(ECALUDP_ENABLE_NPCAP)
    list(APPEND includes
        include_with_udpcap/ecaludp/npcap_helpers.h
        include_with_udpcap/ecaludp/socket_npcap.h
    )

    list(APPEND sources
        src/async_udpcap_socket.cpp
        src/async_udpcap_socket.h
        src/npcap_helpers.cpp
        src/socket_npcap.cpp
    )
endif()

# Build as library
add_library (${PROJECT_NAME} ${ECALUDP_LIBRARY_TYPE}
    ${includes}
    ${sources}
)

# Generate version defines
configure_file("ecaludp_version.h.in" "${PROJECT_BINARY_DIR}/include/ecaludp/ecaludp_version.h" @ONLY)

# Generate header with export macros
generate_export_header(${PROJECT_NAME}
  EXPORT_FILE_NAME ${PROJECT_BINARY_DIR}/include/ecaludp/ecaludp_export.h
  BASE_NAME ECALUDP
)

add_library (ecaludp::ecaludp ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        asio::asio
    PRIVATE
        # Link header-only libs (recycle) as described in this workaround:
        # https://gitlab.kitware.com/cmake/cmake/-/issues/15415#note_633938
        $<BUILD_INTERFACE:steinwurf::recycle>
        $<$<BOOL:${WIN32}>:ws2_32>
        $<$<BOOL:${WIN32}>:wsock32>
        $<$<BOOL:${ECALUDP_ENABLE_NPCAP}>:udpcap::udpcap>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        ASIO_STANDALONE
        _WIN32_WINNT=0x0601
    PUBLIC
		$<$<BOOL:${ECALUDP_ENABLE_NPCAP}>:ECALUDP_UDPCAP_ENABLED>
)

# Check if ecaludp is a static lib. We can only add the private tests for
# static libs and object libs, as we need to have access to the private
# implementation details.
get_target_property(ecaludp_target_type ecaludp TYPE)
if (ecaludp_target_type STREQUAL OBJECT_LIBRARY)
    target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        ECALUDP_STATIC_DEFINE
    )
endif()

# If this library is NOT a shared lib (i.e. it is a static or object lib), we
# need to re-add the DELAYLOAD wpcap.dll. Usually this is exported by udpcap,
# but we link against that PRIVATE, so we need to re-add this specific setting,
# that must be carried along the linkage until we hit a DLL or EXE.
if (${ECALUDP_ENABLE_NPCAP})
    if (NOT ecaludp_target_type STREQUAL SHARED_LIBRARY)
        set_target_properties(${PROJECT_NAME}
            PROPERTIES INTERFACE_LINK_OPTIONS
            -DELAYLOAD:wpcap.dll
        )
    endif()
endif()

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)

target_compile_options(${PROJECT_NAME} PRIVATE
                           $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
                                -Wall -Wextra>
                           $<$<CXX_COMPILER_ID:MSVC>:
                                /W4>)


# Add own public include directory
target_include_directories(${PROJECT_NAME}
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>          # To find the export file generated by generate_export_header
    $<INSTALL_INTERFACE:include>
  PRIVATE
    "src/"
)

# udpcap enabled includes
target_include_directories(${PROJECT_NAME}
  PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include_with_udpcap>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    FOLDER ecal/udp
)

##################################

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${includes}
    ${sources}
)






################################################################################
### Installation rules
################################################################################

set(ECALUDP_INSTALL_CMAKE_DIR "lib/cmake/ecaludp")

# Install Runtime & Libs
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ecaludpTargets
  
  RUNTIME
    DESTINATION "bin"
    COMPONENT ecaludp_runtime

  LIBRARY
    DESTINATION "lib"
    COMPONENT ecaludp_runtime

  ARCHIVE
    DESTINATION "lib"
    COMPONENT ecaludp_dev
)

# Install public header files (-> dev package)
install(
  DIRECTORY "include/ecaludp"
  DESTINATION "include"
  COMPONENT ecaludp_dev
  FILES_MATCHING PATTERN "*.h"
)

if(ECALUDP_ENABLE_NPCAP)
    install(
        DIRECTORY "include_with_udpcap/ecaludp"
        DESTINATION "include"
        COMPONENT ecaludp_dev
        FILES_MATCHING PATTERN "*.h"
    )
endif()

# Install the auto-generated header with the export macros (-> dev package)
install(
  DIRECTORY "${PROJECT_BINARY_DIR}/include/ecaludp"
  DESTINATION "include"
  COMPONENT ecaludp_dev
  FILES_MATCHING PATTERN "*.h"
)

install(
  EXPORT ecaludpTargets 
  FILE ecaludpTargets.cmake 
  DESTINATION ${ECALUDP_INSTALL_CMAKE_DIR}
  NAMESPACE ecaludp::
  COMPONENT ecaludp_dev
)

# Create and install Config.cmake file (-> dev package)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "../cmake/ecaludpConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_/ecaludpConfig.cmake"
  INSTALL_DESTINATION ${ECALUDP_INSTALL_CMAKE_DIR}
  PATH_VARS ECALUDP_INSTALL_CMAKE_DIR
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_/ecaludpConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_/ecaludpConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_/ecaludpConfigVersion.cmake"
  DESTINATION ${ECALUDP_INSTALL_CMAKE_DIR}
  COMPONENT ecaludp_dev
)