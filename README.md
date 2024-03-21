[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

[![Windows](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-windows.yml/badge.svg)](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-windows.yml) [![Ubuntu](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-ubuntu.yml/badge.svg)](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-ubuntu.yml) [![macOS](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-macos.yml/badge.svg)](https://github.com/eclipse-ecal/ecaludp/actions/workflows/build-macos.yml)

# ecaludp

ecaludp is the underlying implementation for UDP traffic in eCAL. It transparently **fragments and reassembles** messages to provide support for big messages. An ecaludp socket is **not limited** to the ordinary UDP datagram size of ~64KiB. It can transport messages **up to 4 GiB**.

ecaludp has **npcap support** for efficient receiving of multicast traffic in Windows. For that, the [udpcap](https://github.com/eclipse-ecal/udpcap) library is used.

ecaludp requires C++14.

## Sample Projects

ecalupd features an asio-style API. Check out the following samples to see its usage:

- [ecaludp_sample](samples/ecaludp_sample/src/main.cpp): The regular ecaludp socket API

- [ecaludp_npcap_sample](samples/ecaludp_sample_npcap/src/main.cpp) The (slightly different) npcap socket API. Only available for Windows.

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

## How to checkout and build

1. Install cmake and git / git-for-windows

2. Checkout this repo and the asio submodule
    ```console
    git clone https://github.com/eclipse-ecal/ecaludp.git
    cd ecaludp
    git submodule init
    git submodule update
    ```

3. CMake the project _(check the next section for available CMake options)_
    ```console
    mkdir _build
    cd _build
    cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=_install
    ```

4. Build the project
    - Linux: `make`
    - Windows: Open `_build\ecaludp.sln` with Visual Studio and build the example project

5. Check the functionality with the `udpcap_sample /.exe` sample project!

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

## Protocol Specification (Version 5)

An ecaludp message consists of one or multiple datagrams. How many datagrams that will be is determined by the fragmentation.

Each datagram carries a header starting at byte 0. The header is defined in [header_v5.h](ecaludp/src/protocol/header_v5.h). Alien datagrams can be eliminated by comparing the `magic` bytes with a predefined value.
Some datagrams may also carry payload directly after the header.

| size   | Name       | Explanation                                             |
|--------|------------|---------------------------------------------------------|
| 32 bit | `magic`      | User-defined binary data. Used for identifying and dropping alien traffic. |
| 8 bit  | `version`    | Header version. Must be `5` for protocol version 5     |
| 24 bit | _reserved_ | Must be sent as 0.     |
| 32 bit <br> little-endian | `type` | Datagram type. Must be one of: <br> `1`: fragmented_message_info <br> `2`: fragment <br> `3`: non_fragmented_message   |
| 32 bit <br> signed little-endian | `id`       | Random ID to match fragmented parts of a message. Used differently depending on the message type:<br> - `type == fragmented_message_info (1)`: The Random ID that this fragmentation info will be applied to <br> - `type == fragment (2)`: The Random ID that this fragment belongs to. Used to match fragments to their fragmentation info <br> - `type == non_fragmented_message (3)`: Unused field. Must be sent as -1. Must not be evaluated.|
| 32 bit <br> unsigned little-endian | `num` | Fragment number. Used differently depending on the datagram type: <br> - `type == fragmented_message_info (1)`: Amount of fragments that this message was split into. <br> - `type == fragment (2)`: The number of this fragment <br> - `type == non_fragmented_message (3)`: Unused field. Must be sent as `1`. Must not be evaluated. |
| 32 bit <br> unsigned little-endian | `len` | Payload length. Used differently depending on the datagram type. The payload must start directly after the header. <br> - `type == fragmented_message_info (1)`: Length of the original message before it got fragmented. Messages of this type must not carry any payload themselves. <br> - `type == fragment (2)`: The payload lenght of this fragment <br> - `type == non_fragmented_message (3)`: The payload length of this message |
| `len` bytes | _payload_ | Payload of the message or fragment.

### Message Types

There are two different types of messages that can be sent: Messages that are fragmented and messages that are not fragmented.

1. **Non-fragmented data**
   - The entire message **consists of 1 datagram** carrying both a header and the payload.
   - The header looks as follows:
        - `type` is set to `non_fragmented_message (3)`
        - `id` is `-1`
        - `num` is `1`
        - `length` is the amount of payload bytes following after the header

2. **Fragmented data**

    A message which had to be fragmented into $n \in \mathbb{N}_0$ parts **consists of $n+1$ datagrams**:

    - **$1\times$ Fragmentation info**
        - The first datagram carries the fragmentation info.
        - The header looks as follows:
            - `type` is set to `fragmented_message_info (1)`
            - `id` is a random number. It is used to match the fragments to their fragmentation info and therefore must be unique for each fragmented message.
            - `num` is the amount of fragments that the message was split into (i.e. $n$)
            - `length` is the length of the original message before it got fragmented
        - This datagram must not carry any payload.

    - **$n\times$ Fragments**
        - The following $n$ datagrams carry the fragments.
        - The header looks as follows:
            - `type` is set to `fragment (2)`
            - `id` is the random number that was used in the fragmentation info
            - `num` is the number of this fragment (i.e. $1$ to $n$)
            - `length` is the length of the payload of this fragment.
        - The payload of each fragment is a part of the original message.

### Communication diagram

The following diagram shows the communication between the sender and the receiver. The sender sends a non-fragmented message and a fragmented message. The fragmented message consists of n fragments.

```
  Sender                              Receiver
    |                                    |
    |  Non-fragmented message            |
    |----------------------------------->|
    |                                    |
    |  Fragmentation info (n fragments)  |
    |----------------------------------->|
    |                                    |
    |  Fragment 1                        |
    |----------------------------------->|
    |                                    |
    |  Fragment 2                        |
    |----------------------------------->|
    |                                    |
    |  ...                               |
    |----------------------------------->|
    |                                    |
    |  Fragment (n)                      |
    |----------------------------------->|
    |                                    |
    |                                    |
```