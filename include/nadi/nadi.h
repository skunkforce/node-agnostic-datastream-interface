/**
 * @file nadi.h
 * @brief Node Agnostic Datastream Interface (NADI) API for datastreaming nodes.
 * @author Odin Holmes
 * @email odinthenerd@gmail.com
 *
 * The Node Agnostic Datastream Interface (NADI) provides a minimalistic API for
 * datastream producers and consumers to interact as nodes in a directional graph.
 * It is designed to be language- and platform-agnostic, using a JSON meta + binary
 * data pattern for extensibility and efficiency. Nodes can operate without assumptions
 * about each other's implementation, enabling flexible and scalable datastreams.
 */

#ifndef NADI_H
#define NADI_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Unique 64-bit identifier for a node, unique per DLL load. */
typedef uint64_t nadi_node_handle;

/** Status codes for NADI operations. */
typedef enum {
    NADI_OK = 0,               /**< Operation successful. */
    NADI_INVALID_NODE = -1,    /**< Invalid or uninitialized node. */
    NADI_INVALID_MESSAGE = -2, /**< Invalid or malformed message. */
    NADI_NOT_INITIALIZED = -3, /**< Node not created. */
    NADI_INVALID_CHANNEL = -4, /**< Invalid channel for the receiver node. */
    NADI_BUFFER_TOO_SMALL = -5 /**< Provided buffer is too small for descriptor. */
} nadi_status;

/** Forward declaration for message struct. */
struct nadi_message;

/** Callback for receiving upstream messages from downstream nodes. Always calls msg->free. Errors should be handled internally (e.g., logging). */
typedef void(*nadi_receive_callback)(struct nadi_message*);
/** Callback for freeing message resources, called by nadi_send (on success) or nadi_receive_callback. Must not be NULL. */
typedef void(*nadi_free_callback)(struct nadi_message*);

/**
 * Message structure for datastream communication, used for both downstream (via nadi_send)
 * and upstream (via nadi_receive_callback) communication.
 * Combines JSON metadata with binary data.
 */
struct nadi_message {
    const char* meta;        /**< Null-terminated JSON string, allocated by sender, freed by nadi_send (on success) or nadi_receive_callback. */
    uint64_t meta_hash;      /**< Hash of meta for quick comparison, 0 is unused. */
    void* data;              /**< Raw bytes, allocated by sender, freed by nadi_send (on success) or nadi_receive_callback. */
    unsigned int data_length;/**< Length of data in bytes. */
    unsigned int channel;    /**< Channel number for multiplexing streams. Most nodes reserve 0xF100 for a "configuration" channel (input/output) and may support 0xF000 for a "configure context" output channel. The context node (handle 0) uses 0xF000 as an input channel for commands. Channels above 0xF000 are reserved for future standardization; user-defined channels must be 0 to 0xF000. */
    nadi_free_callback free; /**< Non-NULL callback to free meta and data, set to nadi_free for upstream messages. */
    nadi_node_handle node;   /**< Sender's node identifier. */
};

/**
 * Creates a node with a callback for receiving upstream messages.
 * @param node Output parameter for the node identifier.
 * @param receive_callback Function to handle upstream messages, or NULL if not receiving.
 * @return NADI_OK on success, or an error code.
 */
DLL_EXPORT nadi_status nadi_create(nadi_node_handle* node, nadi_receive_callback receive_callback);

/**
 * Destroys a node, freeing resources.
 * @param node The node identifier to destroy.
 * @return NADI_OK on success, or an error code.
 */
DLL_EXPORT nadi_status nadi_destroy(nadi_node_handle node);

/**
 * Sends a downstream message to the specified receiver node, taking ownership of the message on success.
 * Validates message->channel against the receiver's input channel numbers (from nadi_descriptor's "channels.input").
 * Assumes message->node (sender) is set correctly by the caller; incorrect values may cause undefined behavior.
 * On success, the message's nadi_free_callback (never NULL) is called when done.
 * On failure (including NADI_INVALID_CHANNEL), the caller retains ownership and must free the message.
 * The caller must not access or free the message after a successful call.
 * The sender's node is identified by message->node, and the receiver by the node parameter.
 * @param message The message to send.
 * @param node The receiver's node identifier.
 * @return NADI_OK on success, or an error code (e.g., NADI_INVALID_CHANNEL).
 */
DLL_EXPORT nadi_status nadi_send(struct nadi_message* message, nadi_node_handle node);

/**
 * Frees a message and its resources.
 * Used as the nadi_free_callback for upstream messages passed to nadi_receive_callback,
 * or can be used as a nadi_free_callback for downstream messages.
 * @param message The message to free.
 */
DLL_EXPORT void nadi_free(struct nadi_message* message);

/**
 * Writes a JSON string describing the node's version, channels, and metadata to the provided buffer.
 * The buffer must be large enough to hold the JSON string, including the null terminator.
 * The length parameter is updated to the actual length of the JSON string (including null terminator).
 * The JSON includes:
 * - "version": Node-specific version (string).
 * - "nadi version": NADI interface version in semantic versioning format (e.g., "1.0.0").
 * - "channels": Object with "input" and "output" arrays of channel descriptions.
 * - Optional fields like "description" (unconstrained, human-readable node description).
 * - Additional top-level fields may be included, with future fields to be standardized.
 * Each channel description has:
 * - "number": Channel number (integer, e.g., 61712 for 0xF100, 61440 for 0xF000).
 * - Optional "description": Human-readable description (string).
 * - Optional "name": Human-readable name (string).
 * - Optional "data types": Array of user-defined data types (e.g., ["json", "microseconds-double"]).
 * Most nodes include an input/output channel with number 0xF100, name "configuration", with optional "data types".
 * Nodes may include an output channel with number 0xF000, name "configure context", requiring only "number" and "name", with optional "data types" (typically ["json"]).
 * The context node (handle 0) includes an input channel 0xF000 for commands.
 * Channels above 0xF000 are reserved for future standardization; user-defined channels must be 0 to 0xF000.
 * Example for a sensor node: {
 *   "version": "1.0.0",
 *   "nadi version": "1.0.0",
 *   "description": "Temperature sensor node",
 *   "channels": {
 *     "input": [
 *       {"number": 61712, "name": "configuration", "data types": ["json"]}
 *     ],
 *     "output": [
 *       {"number": 61712, "name": "configuration"},
 *       {"number": 61440, "name": "configure context"},
 *       {"number": 1, "name": "temperature", "data types": ["json"]}
 *     ]
 *   }
 * }
 * @param buffer Output buffer for the JSON string.
 * @param length In: Size of the buffer; Out: Length of the JSON string (including null terminator).
 * @return NADI_OK if the buffer is large enough, NADI_BUFFER_TOO_SMALL if not, or another error code.
 */
DLL_EXPORT nadi_status nadi_descriptor(char* buffer, size_t* length);

#ifdef __cplusplus
}
#endif

#endif