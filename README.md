# NADI (Node Agnostic Datastream Interface) API Documentation

## Overview

The **NADI** (Node API for Data Interaction) interface provides a minimalistic API for datastreaming nodes to interact as producers and consumers. It is designed to facilitate high-throughput data flow between software components and is compatible with multiple programming languages. NADI follows a meta + data pattern where:

- **Meta**: A null-terminated JSON string that is extensible, supporting layered and versioned functionality.
- **Data**: Raw binary data interpreted based on the meta content, ensuring efficiency.

NADI is versatile and can be used wherever datastreams flow, including communication with physical sensors, data acquisition hardware, analysis algorithms, or time series databases. For real-world applications, see the [nadi_node_interconnect project](https://github.com/skunkforce/nadi_node_interconnect), which enables nodes supporting the NADI interface to be interconnected, forming a data flow graph. The API is lightweight and platform-independent, with support for dynamic loading via DLLs.

## Header File

The interface is defined in `nadi.h`.

## Key Components

### Types

- **`nadi_instance_handle`**: A pointer type (`void*`) serving as a unique identifier for a connection instance. Multiple loadings of a DLL produce distinct instance handles.
- **`nadi_status`**: An integer type representing the status of API operations (e.g., success or error codes).
- **`nadi_receive_callback`**: A function pointer type for callbacks invoked when a message is received. Signature: `void (*nadi_receive_callback)(nadi_message*)`.
- **`nadi_free_callback`**: A function pointer type for callbacks to free a message. Signature: `void (*nadi_free_callback)(nadi_message*)`.

### Structures

#### `nadi_message`

Represents a message exchanged between nodes, containing both meta and data components.

```c
struct nadi_message {
    char* meta;              // Null-terminated JSON string
    unsigned long meta_hash; // Hash of the meta content (0 is unused)
    char* data;              // Raw binary data
    unsigned int data_length;// Length of the data in bytes
    nadi_free_callback free; // Callback to free the message
    nadi_instance_handle instance; // Instance handle for the connection
    unsigned int channel;    // Channel identifier for the message
};
```

- **`meta`**: A JSON string describing the message's structure, version, or other metadata. The JSON object must contain a `"format"` field specifying the format of the `data` field. Examples include `"json"` (indicating `data` is a null-terminated JSON string) and `"microseconds-double"` (indicating `data` is an array of pairs, each with a `uint64_t` for microseconds since 1970 and a `double` for a scalar value in an SI unit).
- **`meta_hash`**: A hash of the meta string for quick comparison. The sender may use any hash algorithm, provided the receiver can assume that two messages from the same instance with the same hash have identical metadata. A value of zero indicates no hash is provided, and the receiver must parse the metadata.
- **`data`**: Binary data interpreted based on the `meta` content.
- **`data_length`**: Size of the `data` buffer in bytes.
- **`free`**: Callback to deallocate the message. This callback may be called from a different thread, requiring thread-safe deallocation of the message and its resources. Care must be taken to avoid blocking to prevent starving the calling thread.
- **`instance`**: The instance handle associated with the message.
- **`channel`**: An identifier for the communication channel.

### Functions

#### `nadi_init`

Initializes a NADI instance and registers a receive callback. This function is reentrant.

```c
DLL_EXPORT nadi_status nadi_init(nadi_instance_handle* instance, nadi_receive_callback callback);
```

- **Parameters**:
  - `instance`: Pointer to store the initialized instance handle.
  - `callback`: Callback function invoked when a message is received. This callback may be called from another thread, so synchronization is required. Care must be taken to avoid blocking within the callback to prevent starving the calling thread on the other side of the interface.
- **Returns**: `nadi_status` indicating success or failure.

#### `nadi_deinit`

Deinitializes a NADI instance, releasing associated resources. This function will block until all threads running on the other side of the interface have finished.

```c
DLL_EXPORT nadi_status nadi_deinit(nadi_instance_handle instance);
```

- **Parameters**:
  - `instance`: The instance handle to deinitialize.
- **Returns**: `nadi_status` indicating success or failure.

#### `nadi_send`

Sends a message to the specified channel, transferring ownership to the other side of the interface. This function is reentrant.

```c
DLL_EXPORT nadi_status nadi_send(nadi_message* message);
```

- **Parameters**:
  - `message`: Pointer to the `nadi_message` to send.
- **Returns**: `nadi_status` indicating success or failure.

#### `nadi_free`

Frees a message using its associated `free` callback. This function is reentrant.

```c
DLL_EXPORT void nadi_free(nadi_message* message);
```

- **Parameters**:
  - `message`: Pointer to the `nadi_message` to free.

#### `nadi_descriptor`

Returns a JSON string describing the node's version, capabilities, and other metadata. This function is reentrant but may block while retrieving the descriptor. The JSON output is extensible but must contain a single object with at least a `"name"` field (string) and a `"version"` field (string). For example:

```json
{"name":"example_node","version":"1.0.0"}
```

```c
DLL_EXPORT char* nadi_descriptor();
```

- **Returns**: A non-owning, null-terminated JSON string. The caller should not modify or free this string.

## Platform Support

- **Windows**: Uses `__declspec(dllexport)` for DLL exports.
- **Other Platforms**: Defines `DLL_EXPORT` as empty for compatibility.
- **C++ Compatibility**: Uses `extern "C"` to ensure C linkage in C++ environments.

## Usage Examples

### C++ Example

```cpp
#include "nadi.h"
#include <nlohmann/json.hpp>
#include <print>
#include <string>
#include <cstdint>

using json = nlohmann::json;

struct MicroDouble {
    uint64_t time;
    double value;
};

void on_free(nadi_message* msg) {
    if (msg) {
        delete[] msg->meta;
        delete[] msg->data;
        delete msg;
    }
}

void on_receive(nadi_message* msg) {
    json meta_json = json::parse(msg->meta);
    std::string format = meta_json["format"].get<std::string>();

    if (format == "microseconds-double") {
        size_t num_pairs = msg->data_length / sizeof(MicroDouble);
        auto* pairs = reinterpret_cast<MicroDouble*>(msg->data);
        for (size_t i = 0; i < num_pairs; ++i) {
            std::print("Pair {}: time={} us, value={}\n", i, pairs[i].time, pairs[i].value);
        }
    } else {
        std::print("Received message: meta={}, data={}\n", msg->meta, msg->data);
    }
    nadi_free(msg);
}

int main() {
    // Check node descriptor
    const char* descriptor = nadi_descriptor();
    json j = json::parse(descriptor);
    if (j["name"] != "example_node") {
        std::println("Unexpected node name: {}", j["name"].get<std::string>());
        return 1;
    }
    std::print("Node descriptor: name={}, version={}\n", j["name"].get<std::string>(), j["version"].get<std::string>());

    // Initialize NADI
    nadi_instance_handle instance;
    nadi_status status = nadi_init(&instance, on_receive);
    if (status != 0) {
        std::println("Initialization failed");
        return 1;
    }

    // Allocate message, meta, and data on the heap
    nadi_message* msg = new nadi_message;
    std::string meta_str = R"({"format":"json","type":"example"})";
    char* meta = new char[meta_str.size() + 1];
    std::strcpy(meta, meta_str.c_str());
    std::string data_str = "Hello, NADI!";
    char* data = new char[data_str.size() + 1];
    std::strcpy(data, data_str.c_str());

    // Initialize message
    msg->meta = meta;
    msg->meta_hash = 12345; // Placeholder; sender can use any hash algorithm
    msg->data = data;
    msg->data_length = static_cast<unsigned int>(data_str.size());
    msg->free = on_free;
    msg->instance = instance;
    msg->channel = 1;

    // Send message, transferring ownership on success
    status = nadi_send(msg);
    if (status != 0) {
        std::println("Send failed");
        on_free(msg); // Free message only on failure
    }

    // Cleanup
    nadi_deinit(instance);
    return 0;
}
```

### Python Example

```python
import ctypes
import json
import struct

# Load the NADI shared library (adjust path as needed)
nadi_lib = ctypes.cdll.LoadLibrary("./libnadi.so")  # or "nadi.dll" on Windows

# Define nadi_message structure
class NadiMessage(ctypes.Structure):
    _fields_ = [
        ("meta", ctypes.c_char_p),
        ("meta_hash", ctypes.c_ulong),
        ("data", ctypes.c_char_p),
        ("data_length", ctypes.c_uint),
        ("free", ctypes.CFUNCTYPE(None, ctypes.POINTER(ctypes.Structure))),
        ("instance", ctypes.c_void_p),
        ("channel", ctypes.c_uint),
    ]

# Define callback types
NadiReceiveCallback = ctypes.CFUNCTYPE(None, ctypes.POINTER(NadiMessage))
NadiFreeCallback = ctypes.CFUNCTYPE(None, ctypes.POINTER(NadiMessage))

# Define function prototypes
nadi_lib.nadi_descriptor.restype = ctypes.c_char_p
nadi_lib.nadi_init.argtypes = [ctypes.POINTER(ctypes.c_void_p), NadiReceiveCallback]
nadi_lib.nadi_init.restype = ctypes.c_int
nadi_lib.nadi_send.argtypes = [ctypes.POINTER(NadiMessage)]
nadi_lib.nadi_send.restype = ctypes.c_int
nadi_lib.nadi_free.argtypes = [ctypes.POINTER(NadiMessage)]
nadi_lib.nadi_deinit.argtypes = [ctypes.c_void_p]
nadi_lib.nadi_deinit.restype = ctypes.c_int

# Free callback
@NadiFreeCallback
def on_free(msg):
    if msg:
        if msg.contents.meta:
            ctypes.c_char_p(msg.contents.meta).value = None
        if msg.contents.data:
            ctypes.c_char_p(msg.contents.data).value = None
        ctypes.c_void_p.from_address(ctypes.addressof(msg)).value = None

# Receive callback
@NadiReceiveCallback
def on_receive(msg):
    meta = msg.contents.meta.decode('utf-8')
    meta_json = json.loads(meta)
    format_type = meta_json["format"]

    if format_type == "microseconds-double":
        num_pairs = msg.contents.data_length // (8 + 8)  # uint64_t + double
        data_bytes = ctypes.string_at(msg.contents.data, msg.contents.data_length)
        for i in range(num_pairs):
            offset = i * 16
            time = struct.unpack_from('<Q', data_bytes, offset)[0]
            value = struct.unpack_from('<d', data_bytes, offset + 8)[0]
            print(f"Pair {i}: time={time} us, value={value}")
    else:
        data = msg.contents.data.decode('utf-8')
        print(f"Received message: meta={meta}, data={data}")

    nadi_lib.nadi_free(msg)

def main():
    # Check node descriptor
    descriptor = nadi_lib.nadi_descriptor()
    descriptor_json = json.loads(descriptor.decode('utf-8'))
    if descriptor_json["name"] != "example_node":
        print(f"Unexpected node name: {descriptor_json['name']}")
        return 1
    print(f"Node descriptor: name={descriptor_json['name']}, version={descriptor_json['version']}")

    # Initialize NADI
    instance = ctypes.c_void_p()
    status = nadi_lib.nadi_init(ctypes.byref(instance), on_receive)
    if status != 0:
        print("Initialization failed")
        return 1

    # Create message
    msg = ctypes.pointer(NadiMessage())
    meta_str = b'{"format":"json","type":"example"}'
    meta = ctypes.c_char_p(meta_str)
    data_str = b"Hello, NADI!"
    data = ctypes.c_char_p(data_str)
    msg.contents.meta = meta
    msg.contents.meta_hash = 12345  # Placeholder; sender can use any hash algorithm
    msg.contents.data = data
    msg.contents.data_length = len(data_str)
    msg.contents.free = on_free
    msg.contents.instance = instance
    msg.contents.channel = 1

    # Send message
    status = nadi_lib.nadi_send(msg)
    if status != 0:
        print("Send failed")
        on_free(msg)  # Free message only on failure

    # Cleanup
    nadi_lib.nadi_deinit(instance)
    return 0

if __name__ == "__main__":
    exit(main())
```

## Notes

- The `meta` JSON string is extensible, allowing nodes to define custom schemas while maintaining compatibility. It must contain a `"format"` field specifying the `data` format, such as `"json"` (indicating a null-terminated JSON string) or `"microseconds-double"` (indicating an array of `uint64_t` and `double` pairs for time and value).
- The `meta_hash` field enables efficient comparison of meta content without string operations. The sender may use any hash algorithm, provided the receiver can assume that two messages from the same instance with the same hash have identical metadata. A `meta_hash` value of zero indicates no hash is provided, requiring the receiver to parse the metadata.
- Callers must ensure that `nadi_free` is called on received messages to prevent memory leaks.
- The `nadi_descriptor` function provides a way to query node capabilities without establishing a connection. It is reentrant but may block while retrieving the descriptor. Its JSON output is extensible but must contain a single object with at least a `"name"` field (string) and a `"version"` field (string). For example: `{"name":"example_node","version":"1.0.0"}`.
- When calling `nadi_send`, ownership of the message is transferred to the other side of the interface on success. The caller should only free the message if `nadi_send` fails.
- The `nadi_init`, `nadi_send`, and `nadi_free` functions are reentrant, allowing them to be safely called from multiple threads concurrently.
- The callback passed to `nadi_init` may be invoked from another thread, requiring proper synchronization in the callback implementation. Blocking operations within the callback should be minimized to avoid starving the calling thread on the other side of the interface.
- The `free` callback in the `nadi_message` structure may be called from a different thread, requiring thread-safe deallocation of the message and its resources. Blocking operations within the callback should be minimized to avoid starving the calling thread.
- The `nadi_deinit` function will block until all threads running on the other side of the interface have finished, ensuring safe cleanup of resources.
