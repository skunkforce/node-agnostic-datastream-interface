#pragma once

#include <nlohmann/json.hpp>
#include <cstdint>
#include <string_view>

namespace nadi::validation {

bool validate_context_abstract_nodes(const nlohmann::json& msg) {
    // Validates context.abstract_nodes message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.abstract_nodes") return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_abstract_nodes_list(const nlohmann::json& msg) {
    // Validates context.abstract_nodes.list message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.abstract_nodes.list") return false;
    if (!msg.contains("instances") || !msg["instances"].is_array()) return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    for (const auto& instance : msg["instances"]) {
        if (!instance.is_object()) return false;
        if (!instance.contains("name") || !instance["name"].is_string()) return false;
        if (!instance.contains("version") || !instance["version"].is_string()) return false;
        if (instance.contains("description") && !instance["description"].is_string()) return false;
        if (instance.contains("channels")) {
            if (!instance["channels"].is_object()) return false;
            if (instance["channels"].contains("input")) {
                if (!instance["channels"]["input"].is_array()) return false;
                for (const auto& channel : instance["channels"]["input"]) {
                    if (!channel.is_object()) return false;
                    if (!channel.contains("number") || !channel["number"].is_number_integer()) return false;
                    if (channel.contains("name") && !channel["name"].is_string()) return false;
                    if (channel.contains("data types") && !channel["data types"].is_array()) return false;
                }
            }
            if (instance["channels"].contains("output")) {
                if (!instance["channels"]["output"].is_array()) return false;
                for (const auto& channel : instance["channels"]["output"]) {
                    if (!channel.is_object()) return false;
                    if (!channel.contains("number") || !channel["number"].is_number_integer()) return false;
                    if (channel.contains("name") && !channel["name"].is_string()) return false;
                    if (channel.contains("data types") && !channel["data types"].is_array()) return false;
                }
            }
        }
    }
    return true;
}

bool validate_context_connect(const nlohmann::json& msg) {
    // Validates context.connect message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.connect") return false;
    if (!msg.contains("source") || !msg["source"].is_array() || msg["source"].size() != 2) return false;
    if (!msg.contains("destination") || !msg["destination"].is_array() || msg["destination"].size() != 2) return false;
    if (!(msg["source"][0].is_string() || msg["source"][0].is_number_integer())) return false;
    if (!msg["source"][1].is_number_integer()) return false;
    if (!(msg["destination"][0].is_string() || msg["destination"][0].is_number_integer())) return false;
    if (!msg["destination"][1].is_number_integer()) return false;
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_connect_confirm(const nlohmann::json& msg) {
    // Validates context.connect.confirm message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.connect.confirm") return false;
    if (!msg.contains("status") || !msg["status"].is_string()) return false;
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_connections(const nlohmann::json& msg) {
    // Validates context.connections message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.connections") return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_connections_list(const nlohmann::json& msg) {
    // Validates context.connections.list message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.connections.list") return false;
    if (!msg.contains("connections") || !msg["connections"].is_array()) return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    for (const auto& conn : msg["connections"]) {
        if (!conn.is_object()) return false;
        if (!conn.contains("source") || !conn["source"].is_array() || conn["source"].size() != 2) return false;
        if (!conn.contains("target") || !conn["target"].is_array() || conn["target"].size() != 2) return false;
        if (!(conn["source"][0].is_string() || conn["source"][0].is_number_integer())) return false;
        if (!conn["source"][1].is_number_integer()) return false;
        if (!(conn["target"][0].is_string() || conn["target"][0].is_number_integer())) return false;
        if (!conn["target"][1].is_number_integer()) return false;
    }
    return true;
}

bool validate_context_disconnect(const nlohmann::json& msg) {
    // Validates context.disconnect message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.disconnect") return false;
    if (!msg.contains("source") || !msg["source"].is_array() || msg["source"].size() != 2) return false;
    if (!msg.contains("destination") || !msg["destination"].is_array() || msg["destination"].size() != 2) return false;
    if (!(msg["source"][0].is_string() || msg["source"][0].is_number_integer())) return false;
    if (!msg["source"][1].is_number_integer()) return false;
    if (!(msg["destination"][0].is_string() || msg["destination"][0].is_number_integer())) return false;
    if (!msg["destination"][1].is_number_integer()) return false;
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_disconnect_confirm(const nlohmann::json& msg) {
    // Validates context.disconnect.confirm message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.disconnect.confirm") return false;
    if (!msg.contains("status") || !msg["status"].is_string()) return false;
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_node_create(const nlohmann::json& msg) {
    // Validates context.node.create message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.node.create") return false;
    if (!msg.contains("abstract_name") || !msg["abstract_name"].is_string()) return false;
    if (!msg.contains("instance_name") || !msg["instance_name"].is_string()) return false;
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_node_create_confirm(const nlohmann::json& msg) {
    // Validates context.node.create.confirm message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.node.create.confirm") return false;
    if (!msg.contains("node") || !msg["node"].is_number_integer()) return false;
    if (!msg.contains("instance_name") || !msg["instance_name"].is_string()) return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_node_destroy(const nlohmann::json& msg) {
    // Validates context.node.destroy message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.node.destroy") return false;
    if (!msg.contains("instance_name") || !msg["instance_name"].is_string()) return false;
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_node_destroy_confirm(const nlohmann::json& msg) {
    // Validates context.node.destroy.confirm message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.node.destroy.confirm") return false;
    if (!msg.contains("status") || !msg["status"].is_string()) return false;
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_nodes(const nlohmann::json& msg) {
    // Validates context.nodes message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.nodes") return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    return true;
}

bool validate_context_nodes_list(const nlohmann::json& msg) {
    // Validates context.nodes.list message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "context.nodes.list") return false;
    if (!msg.contains("instances") || !msg["instances"].is_array()) return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    for (const auto& instance : msg["instances"]) {
        if (!instance.is_object()) return false;
        if (!instance.contains("instance") || !instance["instance"].is_string()) return false;
    }
    return true;
}

bool validate_node_connect(const nlohmann::json& msg) {
    // Validates node.connect message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "node.connect") return false;
    if (!msg.contains("source") || !msg["source"].is_array() || msg["source"].size() != 2) return false;
    if (!msg.contains("target") || !msg["target"].is_number_integer()) return false;
    for (const auto& item : msg["source"]) {
        if (!item.is_number_integer()) return false;
    }
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_node_connect_confirm(const nlohmann::json& msg) {
    // Validates node.connect.confirm message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "node.connect.confirm") return false;
    if (!msg.contains("status") || !msg["status"].is_string()) return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    if (msg.contains("message") && !msg["message"].is_string()) return false;
    return true;
}

bool validate_node_disconnect(const nlohmann::json& msg) {
    // Validates node.disconnect message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "node.disconnect") return false;
    if (!msg.contains("source") || !msg["source"].is_array() || msg["source"].size() != 2) return false;
    if (!msg.contains("target") || !msg["target"].is_number_integer()) return false;
    for (const auto& item : msg["source"]) {
        if (!item.is_number_integer()) return false;
    }
    if (msg.contains("id") && !msg["id"].is_string()) return false;
    return true;
}

bool validate_node_disconnect_confirm(const nlohmann::json& msg) {
    // Validates node.disconnect.confirm message
    if (!msg.is_object()) return false;
    if (!msg.contains("type") || !msg["type"].is_string() || msg["type"] != "node.disconnect.confirm") return false;
    if (!msg.contains("status") || !msg["status"].is_string()) return false;
    if (!msg.contains("id") || !msg["id"].is_string()) return false;
    if (msg.contains("message") && !msg["message"].is_string()) return false;
    return true;
}

} // namespace nadi::validation