#include "protocol/BinaryCommandReader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace protocol {

void write_order_commands_binary(const std::filesystem::path& path,
                                 const std::vector<DecodedOrderCommand>& commands) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("unable to open binary command file for writing: " +
                                 path.string());
    }

    for (const auto& decoded : commands) {
        const auto message = encode_order_command(decoded.command, decoded.timestamp);
        output.write(reinterpret_cast<const char*>(message.data()),
                     static_cast<std::streamsize>(message.size()));
        if (!output) {
            throw std::runtime_error("unable to write binary command file: " + path.string());
        }
    }

    output.close();
    if (!output) {
        throw std::runtime_error("unable to close binary command file after writing: " +
                                 path.string());
    }
}

std::vector<DecodedOrderCommand> read_order_commands_binary(const std::filesystem::path& path) {
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

    std::vector<DecodedOrderCommand> commands;
    commands.reserve(static_cast<std::size_t>(file_size / kMessageSize));

    for (std::uintmax_t index = 0; index < file_size / kMessageSize; ++index) {
        BinaryMessage message{};
        input.read(reinterpret_cast<char*>(message.data()),
                   static_cast<std::streamsize>(message.size()));
        if (!input) {
            throw std::runtime_error("unable to read binary command message from: " +
                                     path.string());
        }

        try {
            commands.emplace_back(decode_order_command(message));
        } catch (const std::invalid_argument& error) {
            std::ostringstream message_stream;
            message_stream << "invalid binary command message at index " << index << ": "
                           << error.what();
            throw std::invalid_argument(message_stream.str());
        }
    }

    return commands;
}

}  // namespace protocol
