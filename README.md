[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# ecaludp

ecaludp is the underlying implementation for UDP traffic in eCAL. It transparently fragments and reassembles messages to provide support for big messages. 

## Dependencies

The following dependencies are required to build ecaludp:

| **Dependency** | **License** | **Integration** |
|----------------|-------------|-----------------|
| [asio](https://github.com/chriskohlhoff/asio) | [Boost Software License](https://github.com/chriskohlhoff/asio/blob/master/asio/LICENSE_1_0.txt) | [git submodule](https://github.com/eclipse-ecal/ecaludp/tree/master/thirdparty) |
| [recycle](https://github.com/steinwurf/recycle) | [BSD-3](https://github.com/steinwurf/recycle/blob/master/LICENSE.rst) | [git submodule](https://github.com/eclipse-ecal/ecaludp/tree/master/thirdparty) |

## CMake Options

You can set the following CMake Options to control how ecaludp is built:

|**Option**                       | **Type** | **Default** | **Explanation**                                                                                                 |
|---------------------------------|----------|-------------|-----------------------------------------------------------------------------------------------------------------|
| `ECALUDP_BUILD_SAMPLES` | `BOOL` | `ON` | Build the ecaludp sample project.                                                                         |
| `ECALUDP_BUILD_TESTS` | `BOOL` | `OFF` | Build the the ecaludp tests. Requires gtest to be available. If ecaludp is built as static or object library, additional tests will be built that test the internal implementation that is not available as public API. |
| `ECALUDP_USE_BUILTIN_ASIO`| `BOOL`| `ON` | Use the builtin asio submodule. If set to `OFF`, asio must be available from somewhere else (e.g. system libs). |
| `ECALUDP_USE_BUILTIN_RECYCLE`| `BOOL`| `ON` | Use the builtin steinwurf::recycle submodule. If set to `OFF`, recycle must be available from somewhere else (e.g. system libs). |
| `ECALUDP_USE_BUILTIN_GTEST`| `BOOL`| `ON` <br>_(when building tests)_ | Use the builtin GoogleTest submodule. Only needed if `FINEFTP_SERVER_BUILD_TESTS` is `ON`. If set to `OFF`, GoogleTest must be available from somewhere else (e.g. system libs). |
| `ECALUDP_LIBRARY_TYPE` | `STRING` |             | Controls the library type of ecaludp. Currently supported are `STATIC` /  `SHARED` / `OBJECT`. If set, this will override the regular `BUILD_SHARED_LIBS` CMake option. If not set, that option will be used. |