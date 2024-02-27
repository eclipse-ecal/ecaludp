[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

[![Windows](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-windows.yml/badge.svg)](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-windows.yml) [![Ubuntu](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-ubuntu.yml/badge.svg)](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-ubuntu.yml) [![macOS](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-macos.yml/badge.svg)](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-macos.yml)

# ecaludp

ecaludp is the underlying implementation for UDP traffic in eCAL. It transparently fragments and reassembles messages to provide support for big messages. 

## Dependencies

The following dependencies are always required to build ecaludp:

| **Dependency** | **License** | **Default Integration** |
|----------------|-------------|-------------------------|
| [asio](https://github.com/chriskohlhoff/asio) | [Boost Software License](https://github.com/chriskohlhoff/asio/blob/master/asio/LICENSE_1_0.txt) | [git submodule](https://github.com/eclipse-ecal/ecaludp/tree/master/thirdparty) |
| [recycle](https://github.com/steinwurf/recycle) | [BSD-3](https://github.com/steinwurf/recycle/blob/master/LICENSE.rst) | [git submodule](https://github.com/eclipse-ecal/ecaludp/tree/master/thirdparty) |

Additionally, when building with **Npcap**, the following dependencies are required:

| **Dependency** | **License** | **Default Integration** |
|----------------|-------------|-------------------------|
| [Udpcap](https://github.com/eclipse-ecal/udpcap) | [Apache 2.0](https://github.com/eclipse-ecal/udpcap/blob/master/LICENSE) | [git submodule](https://github.com/eclipse-ecal/ecaludp/tree/master/thirdparty) |
| [npcap](https://npcap.com/)<br>(_SDK only_)| [Npcap License](https://github.com/nmap/npcap/blob/master/LICENSE) | Fetched by CMake |
| [Pcap++](https://pcapplusplus.github.io/)| [Unlicense](https://github.com/seladb/PcapPlusPlus/blob/master/LICENSE) | Fetched by CMake |

When building the **tests**, the following dependency is required:

| **Dependency** | **License** | **Default Integration** |
|----------------|-------------|-------------------------|
| [Googletest](https://github.com/google/googletest) | [BSD-3](https://github.com/google/googletest/blob/main/LICENSE) | [git submodule](https://github.com/eclipse-ecal/ecaludp/tree/master/thirdparty) |

## CMake Options

You can set the following CMake Options to control how ecaludp is built:

|**Option**                       | **Type** | **Default** | **Explanation**                                                                                                 |
|---------------------------------|----------|-------------|-----------------------------------------------------------------------------------------------------------------|
| `ECALUDP_ENABLE_NPCAP` | `BOOL` | `OFF` | Enable the NPCAP based socket emulation to receive UDP data without actually opening a socket.|
| `ECALUDP_BUILD_SAMPLES` | `BOOL` | `ON` | Build the ecaludp sample project.                                                                         |
| `ECALUDP_BUILD_TESTS` | `BOOL` | `OFF` | Build the the ecaludp tests. Requires gtest to be available. If ecaludp is built as static or object library, additional tests will be built that test the internal implementation that is not available as public API. |
| `ECALUDP_USE_BUILTIN_ASIO`| `BOOL`| `ON` | Use the builtin asio submodule. If set to `OFF`, asio must be available from somewhere else (e.g. system libs). |
| `ECALUDP_USE_BUILTIN_RECYCLE`| `BOOL`| `ON` | Use the builtin steinwurf::recycle submodule. If set to `OFF`, recycle must be available from somewhere else (e.g. system libs). |
| `ECALUDP_USE_BUILTIN_UDPCAP`| `BOOL`| `ON`<br>_(when building with npcap)_ | Use the builtin udpcap submodule. Only needed if `ECALUDP_ENABLE_NPCAP` is `ON`. If set to `OFF`, udpcap must be available from somewhere else (e.g. system libs). Setting this option to `ON` will also use the default dependencies of udpcap (npcap-sdk, pcapplusplus). |
| `ECALUDP_USE_BUILTIN_GTEST`| `BOOL`| `ON` <br>_(when building tests)_ | Use the builtin GoogleTest submodule. Only needed if `FINEFTP_SERVER_BUILD_TESTS` is `ON`. If set to `OFF`, GoogleTest must be available from somewhere else (e.g. system libs). |
| `ECALUDP_LIBRARY_TYPE` | `STRING` |             | Controls the library type of Ecaludp by injecting the string into the `add_library` call. Can be set to STATIC / SHARED / OBJECT. If set, this will override the regular `BUILD_SHARED_LIBS` CMake option. If not set, CMake will use the default setting, which is controlled by `BUILD_SHARED_LIBS`. |