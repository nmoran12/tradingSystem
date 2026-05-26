#include "protocol/BinaryProtocol.hpp"

#include <stdexcept>
#include <string>

namespace protocol {
namespace {

uint8_t read_u8(const BinaryMessage& message, std::size_t offset) {
    return static_cast<uint8_t>(message[offset]);
}

uint32_t read_u32_le(const BinaryMessage& message, std::size_t offset) {
    return static_cast<uint32_t>(read_u8(message, offset)) |
           (static_cast<uint32_t>(read_u8(message, offset + 1)) << 8) |
           (static_cast<uint32_t>(read_u8(message, offset + 2)) << 16) |
           (static_cast<uint32_t>(read_u8(message, offset + 3)) << 24);
}

uint64_t read_u64_le(const BinaryMessage& message, std::size_t offset) {
    uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(read_u8(message, offset + i)) << (8 * i);
    }
    return value;
}

int64_t read_i64_le(const BinaryMessage& message, std::size_t offset) {
    return static_cast<int64_t>(read_u64_le(message, offset));
}

void write_u8(BinaryMessage& message, std::size_t offset, uint8_t value) {
    message[offset] = static_cast<std::byte>(value);
}

void write_u32_le(BinaryMessage& message, std::size_t offset, uint32_t value) {
    message[offset] = static_cast<std::byte>(value & 0xFFu);
    message[offset + 1] = static_cast<std::byte>((value >> 8) & 0xFFu);
    message[offset + 2] = static_cast<std::byte>((value >> 16) & 0xFFu);
    message[offset + 3] = static_cast<std::byte>((value >> 24) & 0xFFu);
}

void write_u64_le(BinaryMessage& message, std::size_t offset, uint64_t value) {
    for (std::size_t i = 0; i < 8; ++i) {
        message[offset + i] = static_cast<std::byte>((value >> (8 * i)) & 0xFFu);
    }
}

void write_i64_le(BinaryMessage& message, std::size_t offset, int64_t value) {
    write_u64_le(message, offset, static_cast<uint64_t>(value));
}

uint8_t encode_command_type(matching_engine::OrderCommandType type) {
    switch (type) {
        case matching_engine::OrderCommandType::NewOrder:
            return kCommandTypeNewOrder;
        case matching_engine::OrderCommandType::CancelOrder:
            return kCommandTypeCancelOrder;
        case matching_engine::OrderCommandType::ModifyOrder:
            return kCommandTypeModifyOrder;
    }
    throw std::invalid_argument("unsupported OrderCommandType for binary encoding");
}

uint8_t encode_side(market_data::Side side, matching_engine::OrderCommandType command_type) {
    switch (side) {
        case market_data::Side::BUY:
            return kSideBuy;
        case market_data::Side::SELL:
            return kSideSell;
        case market_data::Side::UNKNOWN:
            if (command_type == matching_engine::OrderCommandType::CancelOrder) {
                return kSideUnknown;
            }
            throw std::invalid_argument(
                "UNKNOWN side is only valid for CancelOrder in binary encoding");
    }
    throw std::invalid_argument("unsupported Side for binary encoding");
}

uint8_t encode_order_type(matching_engine::OrderType order_type) {
    switch (order_type) {
        case matching_engine::OrderType::Limit:
            return kOrderTypeLimit;
        case matching_engine::OrderType::Market:
            return kOrderTypeMarket;
    }
    throw std::invalid_argument("unsupported OrderType for binary encoding");
}

void write_symbol(BinaryMessage& message, const std::string& symbol) {
    if (symbol.size() > kSymbolLength) {
        throw std::invalid_argument("symbol exceeds 16-byte OBK1 limit: " + symbol);
    }

    for (std::size_t i = 0; i < kSymbolLength; ++i) {
        const std::byte value =
            i < symbol.size() ? static_cast<std::byte>(static_cast<unsigned char>(symbol[i]))
                              : std::byte{0};
        message[kOffsetSymbol + i] = value;
    }
}

void require_zero_byte(uint8_t value, const char* field_name) {
    if (value != 0) {
        throw std::invalid_argument(std::string("non-zero reserved byte: ") + field_name);
    }
}

matching_engine::OrderCommandType decode_command_type(uint8_t wire_value) {
    switch (wire_value) {
        case kCommandTypeNewOrder:
            return matching_engine::OrderCommandType::NewOrder;
        case kCommandTypeCancelOrder:
            return matching_engine::OrderCommandType::CancelOrder;
        case kCommandTypeModifyOrder:
            return matching_engine::OrderCommandType::ModifyOrder;
    }
    throw std::invalid_argument("invalid command_type");
}

market_data::Side decode_side(uint8_t wire_value) {
    switch (wire_value) {
        case kSideUnknown:
            return market_data::Side::UNKNOWN;
        case kSideBuy:
            return market_data::Side::BUY;
        case kSideSell:
            return market_data::Side::SELL;
    }
    throw std::invalid_argument("invalid side");
}

matching_engine::OrderType decode_order_type(uint8_t wire_value) {
    switch (wire_value) {
        case kOrderTypeLimit:
            return matching_engine::OrderType::Limit;
        case kOrderTypeMarket:
            return matching_engine::OrderType::Market;
    }
    throw std::invalid_argument("invalid order_type");
}

void validate_side_for_command_type(market_data::Side side,
                                    matching_engine::OrderCommandType command_type) {
    if (command_type == matching_engine::OrderCommandType::CancelOrder) {
        return;
    }
    if (side != market_data::Side::BUY && side != market_data::Side::SELL) {
        throw std::invalid_argument("BUY or SELL required for NewOrder and ModifyOrder");
    }
}

bool is_symbol_content_byte(uint8_t byte) {
    // Printable ASCII; NUL and space are padding-only after left-aligned text.
    return byte >= 0x21 && byte <= 0x7E;
}

bool is_symbol_padding_byte(uint8_t byte) {
    return byte == 0 || byte == 0x20;
}

std::string decode_symbol(const BinaryMessage& message,
                          matching_engine::OrderCommandType command_type) {
    std::string symbol;
    symbol.reserve(kSymbolLength);
    bool in_padding = false;

    for (std::size_t i = 0; i < kSymbolLength; ++i) {
        const uint8_t byte = read_u8(message, kOffsetSymbol + i);
        if (!in_padding) {
            if (is_symbol_padding_byte(byte)) {
                in_padding = true;
            } else if (is_symbol_content_byte(byte)) {
                symbol.push_back(static_cast<char>(byte));
            } else {
                throw std::invalid_argument("invalid symbol byte");
            }
        } else if (!is_symbol_padding_byte(byte)) {
            throw std::invalid_argument("invalid symbol padding");
        }
    }

    if (command_type == matching_engine::OrderCommandType::CancelOrder) {
        return symbol.empty() ? "DEFAULT" : symbol;
    }

    if (symbol.empty()) {
        throw std::invalid_argument("symbol must not be empty");
    }

    return symbol;
}

}  // namespace

BinaryMessage encode_order_command(const matching_engine::OrderCommand& command,
                                   uint64_t timestamp) {
    BinaryMessage message{};
    // message is zero-initialised

    write_u32_le(message, kOffsetMagic, kMagic);
    write_u8(message, kOffsetVersion, kProtocolVersion);
    write_u8(message, kOffsetWireMessageType, kWireMessageTypeOrderCommand);
    write_u8(message, kOffsetHeaderReserved, 0);
    write_u8(message, kOffsetHeaderReserved + 1, 0);

    write_u64_le(message, kOffsetTimestamp, timestamp);
    write_u8(message, kOffsetCommandType, encode_command_type(command.type));
    write_u8(message, kOffsetSide, encode_side(command.side, command.type));
    write_u8(message, kOffsetOrderType, encode_order_type(command.order_type));
    write_u8(message, kOffsetPayloadReserved, 0);
    write_u8(message, kOffsetAlignReserved, 0);
    write_u8(message, kOffsetAlignReserved + 1, 0);
    write_u8(message, kOffsetAlignReserved + 2, 0);
    write_u8(message, kOffsetAlignReserved + 3, 0);

    write_u64_le(message, kOffsetOrderId, command.order_id);
    write_i64_le(message, kOffsetPrice, command.price);
    write_u64_le(message, kOffsetQuantity, command.quantity);
    write_symbol(message, command.symbol);

    return message;
}

DecodedOrderCommand decode_order_command(const BinaryMessage& message) {
    if (message.size() != kMessageSize) {
        throw std::invalid_argument("binary message must be exactly 64 bytes");
    }

    const uint32_t magic = read_u32_le(message, kOffsetMagic);
    if (magic != kMagic) {
        throw std::invalid_argument("invalid magic");
    }

    const uint8_t version = read_u8(message, kOffsetVersion);
    if (version != kProtocolVersion) {
        throw std::invalid_argument("invalid protocol version");
    }

    const uint8_t wire_message_type = read_u8(message, kOffsetWireMessageType);
    if (wire_message_type != kWireMessageTypeOrderCommand) {
        throw std::invalid_argument("invalid wire_message_type");
    }

    require_zero_byte(read_u8(message, kOffsetHeaderReserved), "header_reserved[0]");
    require_zero_byte(read_u8(message, kOffsetHeaderReserved + 1), "header_reserved[1]");

    DecodedOrderCommand decoded;
    decoded.timestamp = read_u64_le(message, kOffsetTimestamp);

    const auto command_type = decode_command_type(read_u8(message, kOffsetCommandType));
    const auto side = decode_side(read_u8(message, kOffsetSide));
    validate_side_for_command_type(side, command_type);

    decoded.command.type = command_type;
    decoded.command.side = side;
    decoded.command.order_type = decode_order_type(read_u8(message, kOffsetOrderType));

    require_zero_byte(read_u8(message, kOffsetPayloadReserved), "payload_reserved");
    for (std::size_t i = 0; i < kSizeAlignReserved; ++i) {
        require_zero_byte(read_u8(message, kOffsetAlignReserved + i), "align_reserved");
    }

    decoded.command.order_id = read_u64_le(message, kOffsetOrderId);
    decoded.command.price = read_i64_le(message, kOffsetPrice);
    decoded.command.quantity = read_u64_le(message, kOffsetQuantity);
    decoded.command.symbol = decode_symbol(message, command_type);

    return decoded;
}

}  // namespace protocol
