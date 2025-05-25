# NADI: Node Agnostic Datastream Interface

## Introduction

The Node Agnostic Datastream Interface (NADI) is both a concrete interface and an abstraction layer for modeling dataflow in complex, distributed systems. As an interface, NADI enables seamless communication between nodes in a directional graph through standardized message passing. As an architecture, it provides a flexible framework for connecting diverse components, such as device drivers, sensors, analysis algorithms, logs, databases, or networked devices, in a hierarchical, scalable manner.

The simplest use case involves a program interacting with a NADI-compatible driver (e.g., a temperature sensor DLL), where the program creates a node, configures it via messages, and receives data through a callback. NADI's architecture supports more complex scenarios, including multi-node graphs and nested contexts, though this document focuses on the core interface and a simple use case.

This document explains NADI's core concepts, terminology, message schemas, and provides examples in C++ and Python for interacting with a temperature sensor driver. For advanced use cases, see the [nadi node interconnect](https://github.com/skunkforce/nadi_node_interconnect).

## Core Concepts

NADI models dataflow as a directional graph of nodes connected by channels:

- **Nodes**: Concrete entities that produce or consume messages, such as a sensor or a log. Each node has input and output channels.
- **Node Handle**: A unique identifier for a node instance, implemented as a 64-bit number in the C ABI (`nadi_node_handle`).
- **Abstract Node**: A class or template (e.g., a sensor driver DLL) from which concrete nodes are created.
- **Channel**: Input and output channels on a node, identified by channel numbers, used for routing messages.
- **Graph**: A collection of nodes with a connection table mapping output channels to input channels.
- **Sub Graph**: A node containing its own graph of nodes, enabling hierarchical dataflow.
- **Context (Hierarchical Context Node)**: A special node (handle `0`) in each graph or sub graph, automatically created, managing the connection table and lists of nodes and abstract nodes.
- **Node Alias**: A user-provided string (e.g., `"sensor1"`) mapped to a node handle by the context.
- **Message**: Data passed between channels, immutable after sending. In the C ABI, messages use the `nadi_message` struct; in other contexts (e.g., websockets), they are JSON objects.

Messages are reference-counted by the context for safe delivery to multiple targets. The context node handles connection routing and node management, accessible via specific channels.

## Terminology
- **Node**: Concrete dataflow entity (e.g., a sensor instance).
- **Node Handle**: Unique identifier (e.g., `uint64_t` in C).
- **Abstract Node**: Template for creating nodes (e.g., a driver DLL).
- **Channel Number**: Identifier for input/output channels (`unsigned int` in C).
- **Graph**: Collection of nodes and connections.
- **Sub Graph**: Nested graph within a node.
- **Context**: Special node (`node: 0`) managing connections and node lists.
- **Node Alias**: Semantic string for a node (e.g., `"sensor1"`).
- **Message**: Data unit, structured as `nadi_message` in C or JSON elsewhere.

## Message Schemas

NADI messages are documented as JSON schemas for clarity, with mappings to the C ABI's `nadi_message` struct (`meta` → JSON string, `data` → bytes). Most messages include an optional `"id"` field for correlation, though some messages (out of scope for this document) may not. Below are AsyncAPI fragments for standardized messages.

### Configuration Messages (Sent to 0xF100)
- **node.connect**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: node.connect
        example: node.connect
      source:
        type: array
        items:
          type: integer
        minItems: 2
        maxItems: 2
        example: [1234, 1234]
      target:
        type: integer
        example: 61712
      id:
        type: string
        example: conn1
    required: [type, source, target]
  ```
- **node.disconnect**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: node.disconnect
        example: node.disconnect
      source:
        type: array
        items:
          type: integer
        minItems: 2
        maxItems: 2
        example: [1234, 1234]
      target:
        type: integer
        example: 61712
      id:
        type: string
        example: disconn1
    required: [type, source, target]
  ```

### Configuration Messages (Sent to 0xF000 of Context Node)
- **context.node.create**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: context.node.create
        example: context.node.create
      abstract_name:
        type: string
        example: sensor_driver
      instance_name:
        type: string
        example: sensor1
      id:
        type: string
        example: create1
    required: [type, abstract_name, instance_name]
  ```
- **context.node.destroy**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: context.node.destroy
        example: context.node.destroy
      instance_name:
        type: string
        example: sensor1
      id:
        type: string
        example: destroy1
    required: [type, instance_name]
  ```
- **context.connect**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: context.connect
        example: context.connect
      source:
        type: array
        items:
          oneOf:
            - type: string
            - type: integer
        minItems: 2
        maxItems: 2
        example: [1234, 1234]
      destination:
        type: array
        items:
          oneOf:
            - type: string
            - type: integer
        minItems: 2
        maxItems: 2
        example: [5678, 61712]
      id:
        type: string
        example: conn2
    required: [type, source, destination]
  ```
- **context.disconnect**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: context.disconnect
        example: context.disconnect
      source:
        type: array
        items:
          oneOf:
            - type: string
            - type: integer
        minItems: 2
        maxItems: 2
        example: [1234, 1234]
      destination:
        type: array
        items:
          oneOf:
            - type: string
            - type: integer
        minItems: 2
        maxItems: 2
        example: [5678, 61712]
      id:
        type: string
        example: disconn2
    required: [type, source, destination]
  ```

### Query Messages (Sent to 0xF000 of Context Node)
- **context.connections**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: context.connections
        example: context.connections
      id:
        type: string
        example: conn_query1
    required: [type, id]
  ```
- **context.abstract_nodes**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: context.abstract_nodes
        example: context.abstract_nodes
      id:
        type: string
        example: abs_nodes1
    required: [type, id]
  ```
- **context.nodes**:
  ```yaml
  schema:
    type: object
    properties:
      type:
        type: string
        const: context.nodes
        example: context.nodes
      id:
        type: string
        example: nodes1
    required: [type, id]
  ```

### Response Messages
- **From 0xF100**:
  - **node.connect.confirm**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: node.connect.confirm
          example: node.connect.confirm
        status:
          type: string
          example: success
        message:
          type: string
          example: Connection established
        id:
          type: string
          example: conn1
      required: [type, status, id]
    ```
  - **node.disconnect.confirm**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: node.disconnect.confirm
          example: node.disconnect.confirm
        status:
          type: string
          example: success
        message:
          type: string
          example: Connection removed
        id:
          type: string
          example: disconn1
      required: [type, status, id]
    ```
  - **context.connect.confirm**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: context.connect.confirm
          example: context.connect.confirm
        status:
          type: string
          example: success
        id:
          type: string
          example: conn2
      required: [type, status]
    ```
  - **context.disconnect.confirm**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: context.disconnect.confirm
          example: context.disconnect.confirm
        status:
          type: string
          example: success
        id:
          type: string
          example: disconn2
      required: [type, status]
    ```
- **From 0xF000**:
  - **context.node.create.confirm**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: context.node.create.confirm
          example: context.node.create.confirm
        node:
          type: integer
          example: 1234
        instance_name:
          type: string
          example: sensor1
        id:
          type: string
          example: create1
      required: [type, node, instance_name, id]
    ```
  - **context.node.destroy.confirm**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: context.node.destroy.confirm
          example: context.node.destroy.confirm
        status:
          type: string
          example: success
        id:
          type: string
          example: destroy1
      required: [type, status]
    ```
  - **context.connections.list**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: context.connections.list
          example: context.connections.list
        connections:
          type: array
          items:
            type: object
            properties:
              source:
                type: array
                items:
                  oneOf:
                    - type: string
                    - type: integer
                minItems: 2
                maxItems: 2
                example: [1234, 1234]
              target:
                type: array
                items:
                  oneOf:
                    - type: string
                    - type: integer
                minItems: 2
                maxItems: 2
                example: [5678, 61712]
        id:
          type: string
          example: conn_query1
      required: [type, connections, id]
    ```
  - **context.abstract_nodes.list**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: context.abstract_nodes.list
          example: context.abstract_nodes.list
        instances:
          type: array
          items:
            type: object
            properties:
              name:
                type: string
                example: sensor_driver
              version:
                type: string
                example: 1.0.0
              description:
                type: string
                example: Temperature sensor driver
              channels:
                type: object
                properties:
                  input:
                    type: array
                    items:
                      type: object
                      properties:
                        number:
                          type: integer
                        name:
                          type: string
                        data types:
                          type: array
                          items:
                            type: string
                  output:
                    type: array
                    items:
                      type: object
                      properties:
                        number:
                          type: integer
                        name:
                          type: string
                        data types:
                          type: array
                          items:
                            type: string
        id:
          type: string
          example: abs_nodes1
      required: [type, instances, id]
    ```
  - **context.nodes.list**:
    ```yaml
    schema:
      type: object
      properties:
        type:
          type: string
          const: context.nodes.list
          example: context.nodes.list
        instances:
          type: array
          items:
            type: object
            properties:
              instance:
                type: string
                example: sensor1
        id:
          type: string
          example: nodes1
      required: [type, instances, id]
    ```

### Response Routing
Responses are sent from:
- `0xF100` (output) for configuration commands sent to a node’s `0xF100`.
- `0xF000` (output) for commands and queries sent to the context node’s `0xF000`.
The context’s connection table routes responses to input channels, requiring users to connect output channels (`0xF100`, `0xF000`) to input channels (out of scope for this document).

### C ABI Mapping
JSON messages map to `nadi_message`:
- `"meta"`: `meta` (JSON string, e.g., `"json"`).
- `"data"`: `data` (serialized JSON or binary).
- `"id"`: Included in `data` JSON.
- `channel`: `channel` (e.g., 61712, 61440).
- `node`: `node` (e.g., context node `0`).

## C++ Example
This example demonstrates a program interacting with a temperature sensor driver DLL using the NADI C ABI.

```cpp
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "nadi.h"

typedef nadi_status (*NADI_CREATE)(nadi_node_handle*, nadi_receive_callback);
typedef nadi_status (*NADI_SEND)(struct nadi_message*, nadi_node_handle);
typedef void (*NADI_FREE)(struct nadi_message*);
typedef nadi_status (*NADI_DESTROY)(nadi_node_handle);

void receive_callback(struct nadi_message* msg) {
    if (strcmp(msg->meta, "json") == 0) {
        printf("Received: %s\n", (char*)msg->data);
    }
    msg->free(msg);
}

int main() {
    HMODULE dll = LoadLibraryA("sensor_driver.dll");
    if (!dll) {
        printf("Failed to load DLL\n");
        return 1;
    }

    NADI_CREATE nadi_create = (NADI_CREATE)GetProcAddress(dll, "nadi_create");
    NADI_SEND nadi_send = (NADI_SEND)GetProcAddress(dll, "nadi_send");
    NADI_FREE nadi_free = (NADI_FREE)GetProcAddress(dll, "nadi_free");
    NADI_DESTROY nadi_destroy = (NADI_DESTROY)GetProcAddress(dll, "nadi_destroy");

    if (!nadi_create || !nadi_send || !nadi_free || !nadi_destroy) {
        printf("Failed to load functions\n");
        FreeLibrary(dll);
        return 1;
    }

    nadi_node_handle context = 0;
    if (nadi_create(&context, receive_callback) != NADI_OK) {
        printf("Failed to create context\n");
        FreeLibrary(dll);
        return 1;
    }

    struct nadi_message* create_msg = (struct nadi_message*)malloc(sizeof(struct nadi_message));
    create_msg->meta = (char*)malloc(256);
    create_msg->data = malloc(256);
    strcpy((char*)create_msg->meta, "json");
    strcpy((char*)create_msg->data, "{\"type\":\"context.node.create\",\"abstract_name\":\"sensor_driver\",\"instance_name\":\"sensor1\",\"id\":\"create1\"}");
    create_msg->data_length = strlen((char*)create_msg->data) + 1;
    create_msg->meta_hash = 0;
    create_msg->channel = 61440; // 0xF000
    create_msg->free = nadi_free;
    create_msg->node = context;
    if (nadi_send(create_msg, context) != NADI_OK) {
        printf("Failed to send create message\n");
        nadi_free(create_msg);
        nadi_destroy(context);
        FreeLibrary(dll);
        return 1;
    }

    struct nadi_message* connect_msg = (struct nadi_message*)malloc(sizeof(struct nadi_message));
    connect_msg->meta = (char*)malloc(256);
    connect_msg->data = malloc(256);
    strcpy((char*)connect_msg->meta, "json");
    strcpy((char*)connect_msg->data, "{\"type\":\"node.connect\",\"source\":[1234,1234],\"target\":61712,\"id\":\"conn1\"}");
    connect_msg->data_length = strlen((char*)connect_msg->data) + 1;
    connect_msg->meta_hash = 0;
    connect_msg->channel = 61712; // 0xF100
    connect_msg->free = nadi_free;
    connect_msg->node = 1234; // Assume sensor node handle
    if (nadi_send(connect_msg, 1234) != NADI_OK) {
        printf("Failed to send connect message\n");
        nadi_free(connect_msg);
        nadi_destroy(context);
        FreeLibrary(dll);
        return 1;
    }

    struct nadi_message* config_msg = (struct nadi_message*)malloc(sizeof(struct nadi_message));
    config_msg->meta = (char*)malloc(256);
    config_msg->data = malloc(256);
    strcpy((char*)config_msg->meta, "json");
    strcpy((char*)config_msg->data, "{\"type\":\"sensor.config\",\"interval\":1000,\"id\":\"config1\"}");
    config_msg->data_length = strlen((char*)config_msg->data) + 1;
    config_msg->meta_hash = 0;
    config_msg->channel = 61712; // 0xF100
    config_msg->free = nadi_free;
    config_msg->node = 1234;
    if (nadi_send(config_msg, 1234) != NADI_OK) {
        printf("Failed to send config message\n");
        nadi_free(config_msg);
        nadi_destroy(context);
        FreeLibrary(dll);
        return 1;
    }

    Sleep(5000); // Wait for messages
    nadi_destroy(context);
    FreeLibrary(dll);
    return 0;
}
```

**Diagram**:
```
Program -> [Sensor:0xF100] -> Callback
```

## Python Example
This mirrors the C++ example using `ctypes`.

```python
import ctypes
import json

class nadi_message(ctypes.Structure):
    _fields_ = [
        ("meta", ctypes.c_char_p),
        ("meta_hash", ctypes.c_uint64),
        ("data", ctypes.c_void_p),
        ("data_length", ctypes.c_uint),
        ("channel", ctypes.c_uint),
        ("free", ctypes.CFUNCTYPE(None, ctypes.POINTER(ctypes.c_void_p))),
        ("node", ctypes.c_uint64)
    ]

def receive_callback(msg):
    if msg.contents.meta.decode() == "json":
        print("Received:", msg.contents.data.decode())
    msg.contents.free(ctypes.byref(msg))

lib = ctypes.WinDLL("sensor_driver.dll")
nadi_create = lib.nadi_create
nadi_create.argtypes = [ctypes.POINTER(ctypes.c_uint64), ctypes.CFUNCTYPE(None, ctypes.POINTER(nadi_message))]
nadi_create.restype = ctypes.c_int
nadi_send = lib.nadi_send
nadi_send.argtypes = [ctypes.POINTER(nadi_message), ctypes.c_uint64]
nadi_send.restype = ctypes.c_int
nadi_free = lib.nadi_free
nadi_free.argtypes = [ctypes.POINTER(nadi_message)]
nadi_free.restype = None
nadi_destroy = lib.nadi_destroy
nadi_destroy.argtypes = [ctypes.c_uint64]
nadi_destroy.restype = ctypes.c_int

context = ctypes.c_uint64(0)
callback = ctypes.CFUNCTYPE(None, ctypes.POINTER(nadi_message))(receive_callback)
if nadi_create(ctypes.byref(context), callback) != 0:
    print("Failed to create context")
    exit(1)

create_msg = nadi_message()
create_msg.meta = b"json"
create_data = json.dumps({"type": "context.node.create", "abstract_name": "sensor_driver", "instance_name": "sensor1", "id": "create1"}).encode()
create_msg.data = ctypes.cast(ctypes.c_char_p(create_data), ctypes.c_void_p)
create_msg.data_length = len(create_data) + 1
create_msg.meta_hash = 0
create_msg.channel = 61440
create_msg.free = ctypes.CFUNCTYPE(None, ctypes.POINTER(ctypes.c_void_p))(nadi_free)
create_msg.node = context.value
if nadi_send(ctypes.byref(create_msg), context.value) != 0:
    print("Failed to send create message")
    nadi_free(ctypes.byref(create_msg))
    nadi_destroy(context.value)
    exit(1)

connect_msg = nadi_message()
connect_msg.meta = b"json"
connect_data = json.dumps({"type": "node.connect", "source": [1234, 1234], "target": 61712, "id": "conn1"}).encode()
connect_msg.data = ctypes.cast(ctypes.c_char_p(connect_data), ctypes.c_void_p)
connect_msg.data_length = len(connect_data) + 1
connect_msg.meta_hash = 0
connect_msg.channel = 61712
connect_msg.free = ctypes.CFUNCTYPE(None, ctypes.POINTER(ctypes.c_void_p))(nadi_free)
connect_msg.node = 1234
if nadi_send(ctypes.byref(connect_msg), 1234) != 0:
    print("Failed to send connect message")
    nadi_free(ctypes.byref(connect_msg))
    nadi_destroy(context.value)
    exit(1)

config_msg = nadi_message()
config_msg.meta = b"json"
config_data = json.dumps({"type": "sensor.config", "interval": 1000, "id": "config1"}).encode()
config_msg.data = ctypes.cast(ctypes.c_char_p(config_data), ctypes.c_void_p)
config_msg.data_length = len(config_data) + 1
config_msg.meta_hash = 0
config_msg.channel = 61712
config_msg.free = ctypes.CFUNCTYPE(None, ctypes.POINTER(ctypes.c_void_p))(nadi_free)
config_msg.node = 1234
if nadi_send(ctypes.byref(config_msg), 1234) != 0:
    print("Failed to send config message")
    nadi_free(ctypes.byref(config_msg))
    nadi_destroy(context.value)
    exit(1)

import time
time.sleep(5)
nadi_destroy(context.value)
```

## Related Projects
- [nadi node interconnect](https://github.com/skunkforce/nadi_node_interconnect): Implements a context for managing multiple NADI nodes.

## Notes
- **Reserved Channels**:
  - `0xF100` (61712): Mandatory input/output for configuration and responses.
  - `0xF000` (61440): Input on context node (`node: 0`), output on all nodes.
  - Above `0xF000` (>61440): Reserved for future standardization.
- **User-Defined Channels**: `0` to `0xF000`, excluding reserved channels.
- **Future Extensions**: Additional top-level fields may be standardized in `nadi_descriptor`.