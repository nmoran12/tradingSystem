#pragma once

#include "protocol/BinaryProtocol.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace protocol {

/// Writes decoded commands as back-to-back fixed-width OBK1 v1 messages.
/// @throws std::runtime_error if the file cannot be opened or written.
/// @throws std::invalid_argument if any command cannot be encoded.
void write_order_commands_binary(const std::filesystem::path& path,
                                 const std::vector<DecodedOrderCommand>& commands);

/// Reads a binary command file containing back-to-back fixed-width OBK1 v1 messages.
/// Empty files are valid and return an empty vector.
/// @throws std::runtime_error if the file cannot be opened or read.
/// @throws std::invalid_argument if the file size or any message is invalid.
std::vector<DecodedOrderCommand> read_order_commands_binary(const std::filesystem::path& path);

/// Streams a binary command file one fixed-width OBK1 v1 message at a time.
/// The callback is invoked immediately after each successful decode.
/// Empty files are valid and return 0.
/// @returns number of decoded commands passed to the callback.
/// @throws std::runtime_error if the file cannot be opened or read.
/// @throws std::invalid_argument if the file size or any message is invalid.
template <typename Callback>
std::size_t stream_order_commands_binary(const std::filesystem::path& path,
                                         Callback&& on_command) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error("unable to open binary command file for reading: " +
                                 path.string());
    }

    const auto end_position = input.tellg();
    if (end_position < 0) {
        throw std::runtime_error("unable to determine binary command file size: " +
                                 path.string());
    }

    const auto file_size = static_cast<std::uintmax_t>(end_position);
    if (file_size % kMessageSize != 0) {
        throw std::invalid_argument("invalid binary command file size: " + path.string());
    }

    input.seekg(0, std::ios::beg);
    if (!input && file_size != 0) {
        throw std::runtime_error("unable to seek binary command file: " + path.string());
    }

    const auto command_count = file_size / kMessageSize;
    for (std::uintmax_t index = 0; index < command_count; ++index) {
        BinaryMessage message{};
        input.read(reinterpret_cast<char*>(message.data()),
                   static_cast<std::streamsize>(message.size()));
        if (!input) {
            throw std::runtime_error("unable to read binary command message from: " +
                                     path.string());
        }

        DecodedOrderCommand decoded;
        try {
            decoded = decode_order_command(message);
        } catch (const std::invalid_argument& error) {
            std::ostringstream message_stream;
            message_stream << "invalid binary command message at index " << index << ": "
                           << error.what();
            throw std::invalid_argument(message_stream.str());
        }
        on_command(decoded);
    }

    return static_cast<std::size_t>(command_count);
}

}  // namespace protocol
