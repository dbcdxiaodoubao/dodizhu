# TCP Framing Gateway Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Windows-native C++17 TCP server that receives and echoes length-prefixed game packets.

**Architecture:** `PacketCodec` owns the six-byte protocol header representation and its validation. `TcpServer` accepts connections, while a `TcpSession` owns one socket and asynchronously reads a header, reads the declared body, then echoes the whole packet. The server contains no game rules or Protobuf parsing yet.

**Tech Stack:** C++17, Boost.Asio, CMake 4.4, CLion MinGW GCC 15.2, vcpkg `x64-mingw-dynamic`, CTest.

---

### Task 1: CMake Project Setup

**Files:**
- Create: `cpp/CMakeLists.txt`
- Create: `cpp/src/main.cpp`

- [ ] **Step 1: Create the CMake target definitions**

```cmake
cmake_minimum_required(VERSION 3.25)
project(doudizhu_gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS asio system)

add_executable(gateway src/main.cpp)
target_link_libraries(gateway PRIVATE Boost::asio Boost::system ws2_32)
```

- [ ] **Step 2: Configure the build**

Run:

```text
cmake -S cpp -B cpp/build -G Ninja -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic
```

Expected: CMake finds Boost and generates Ninja build files.

### Task 2: Packet Codec

**Files:**
- Create: `cpp/include/gateway/packet_codec.hpp`
- Create: `cpp/src/packet_codec.cpp`
- Create: `cpp/tests/packet_codec_test.cpp`
- Modify: `cpp/CMakeLists.txt`

- [ ] **Step 1: Write the failing codec test**

```cpp
auto header = gateway::PacketCodec::decode_header({0, 0, 0, 9, 0, 42});
assert(header.has_value());
assert(header->packet_size == 9);
assert(header->message_id == 42);
```

- [ ] **Step 2: Run the test and verify it fails because `PacketCodec` does not exist**

Run:

```text
cmake --build cpp/build
ctest --test-dir cpp/build --output-on-failure
```

Expected: Compilation failure for missing `gateway::PacketCodec`.

- [ ] **Step 3: Implement the minimum codec**

```cpp
struct PacketHeader {
    std::uint32_t packet_size;
    std::uint16_t message_id;
};

class PacketCodec {
public:
    static std::optional<PacketHeader> decode_header(const std::array<std::uint8_t, 6>& bytes);
};
```

`packet_codec.cpp` implements the declaration, rejects packet sizes below six bytes, and decodes integers in big-endian order. `CMakeLists.txt` adds `packet_codec_test`, links it with the codec source, and registers it with `add_test(NAME packet_codec_test COMMAND packet_codec_test)`.

- [ ] **Step 4: Run the codec test and verify it passes**

Run:

```text
ctest --test-dir cpp/build --output-on-failure
```

Expected: one passing test.

### Task 3: Asynchronous TCP Server

**Files:**
- Create: `cpp/include/gateway/tcp_server.hpp`
- Create: `cpp/src/tcp_server.cpp`
- Modify: `cpp/src/main.cpp`
- Modify: `cpp/CMakeLists.txt`

- [ ] **Step 1: Implement `TcpServer` and `TcpSession`**

`TcpServer` binds `0.0.0.0:9000` and repeatedly calls `async_accept`. A `TcpSession` uses `async_read` to read exactly six header bytes, validates them with `PacketCodec`, then reads `packet_size - 6` body bytes. It concatenates header and body and uses `async_write` to echo the same bytes before reading the next packet.

- [ ] **Step 2: Build the executable**

Run:

```text
cmake --build cpp/build
```

Expected: `cpp/build/gateway.exe` is produced.

- [ ] **Step 3: Run the server**

Run:

```text
cpp/build/gateway.exe
```

Expected: the console reports that TCP port 9000 is listening.

### Task 4: Manual Binary Packet Verification

**Files:**
- No source changes.

- [ ] **Step 1: Send a packet from PowerShell**

```powershell
$client = [System.Net.Sockets.TcpClient]::new('127.0.0.1', 9000)
$stream = $client.GetStream()
[byte[]]$packet = 0,0,0,9,0,42,1,2,3
$stream.Write($packet, 0, $packet.Length)
[byte[]]$response = New-Object byte[] 9
$received = 0
while ($received -lt $response.Length) {
    $read = $stream.Read($response, $received, $response.Length - $received)
    if ($read -eq 0) { throw 'Connection closed before a complete packet arrived.' }
    $received += $read
}
$response -join ','
```

Expected: `0,0,0,9,0,42,1,2,3`, proving the server preserves the binary framing.

- [ ] **Step 2: Stop the server after verification**

Expected: no process remains bound to TCP port 9000.
