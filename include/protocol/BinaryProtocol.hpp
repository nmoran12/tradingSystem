#pragma once

// OBK1 v1 wire layout: docs/BINARY_PROTOCOL.md
// Milestone 5B: encoder. Milestone 5C: decoder.

#include "matching_engine/OrderCommand.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace protocol {

// ---------------------------------------------------------------------------
// Protocol identity
// ---------------------------------------------------------------------------

/// ASCII "OBK1" stored as little-endian uint32 (bytes on wire: 4F 42 4B 31).
inline constexpr uint32_t kMagic = 0x314B424Fu;

/// Wire format version; must match docs/BINARY_PROTOCOL.md.
inline constexpr uint8_t kProtocolVersion = 1;

/// Fixed size of one OrderCommand message (bytes).
inline constexpr std::size_t kMessageSize = 64;

/// Fixed symbol field length (bytes), ASCII padding.
inline constexpr std::size_t kSymbolLength = 16;

// ---------------------------------------------------------------------------
// wire_message_type (header byte at offset 5)
// ---------------------------------------------------------------------------

inline constexpr uint8_t kWireMessageTypeOrderCommand = 0x01;

// ---------------------------------------------------------------------------
// command_type (payload byte at offset 16) → matching_engine::OrderCommandType
// ---------------------------------------------------------------------------

inline constexpr uint8_t kCommandTypeNewOrder = 0x01;
inline constexpr uint8_t kCommandTypeCancelOrder = 0x02;
inline constexpr uint8_t kCommandTypeModifyOrder = 0x03;
inline constexpr uint8_t kCommandTypeInvalid = 0xFF;

// ---------------------------------------------------------------------------
// side (payload byte at offset 17) → market_data::Side
// ---------------------------------------------------------------------------

inline constexpr uint8_t kSideUnknown = 0x00;
inline constexpr uint8_t kSideBuy = 0x01;
inline constexpr uint8_t kSideSell = 0x02;
inline constexpr uint8_t kSideInvalid = 0xFF;

// ---------------------------------------------------------------------------
// order_type (payload byte at offset 18) → matching_engine::OrderType
// ---------------------------------------------------------------------------

inline constexpr uint8_t kOrderTypeLimit = 0x01;
inline constexpr uint8_t kOrderTypeMarket = 0x02;
inline constexpr uint8_t kOrderTypeInvalid = 0xFF;

// ---------------------------------------------------------------------------
// Field offsets (zero-based; see docs/BINARY_PROTOCOL.md)
// ---------------------------------------------------------------------------

inline constexpr std::size_t kOffsetMagic = 0;
inline constexpr std::size_t kOffsetVersion = 4;
inline constexpr std::size_t kOffsetWireMessageType = 5;
inline constexpr std::size_t kOffsetHeaderReserved = 6;
inline constexpr std::size_t kOffsetTimestamp = 8;
inline constexpr std::size_t kOffsetCommandType = 16;
inline constexpr std::size_t kOffsetSide = 17;
inline constexpr std::size_t kOffsetOrderType = 18;
inline constexpr std::size_t kOffsetPayloadReserved = 19;
inline constexpr std::size_t kOffsetAlignReserved = 20;
inline constexpr std::size_t kOffsetOrderId = 24;
inline constexpr std::size_t kOffsetPrice = 32;
inline constexpr std::size_t kOffsetQuantity = 40;
inline constexpr std::size_t kOffsetSymbol = 48;

// ---------------------------------------------------------------------------
// Field sizes
// ---------------------------------------------------------------------------

inline constexpr std::size_t kSizeMagic = 4;
inline constexpr std::size_t kSizeVersion = 1;
inline constexpr std::size_t kSizeWireMessageType = 1;
inline constexpr std::size_t kSizeHeaderReserved = 2;
inline constexpr std::size_t kSizeTimestamp = 8;
inline constexpr std::size_t kSizeCommandType = 1;
inline constexpr std::size_t kSizeSide = 1;
inline constexpr std::size_t kSizeOrderType = 1;
inline constexpr std::size_t kSizePayloadReserved = 1;
inline constexpr std::size_t kSizeAlignReserved = 4;
inline constexpr std::size_t kSizeOrderId = 8;
inline constexpr std::size_t kSizePrice = 8;
inline constexpr std::size_t kSizeQuantity = 8;

// ---------------------------------------------------------------------------
// Compile-time layout validation
// ---------------------------------------------------------------------------

static_assert(kMessageSize == 64, "OBK1 v1 message must be 64 bytes");
static_assert(kSymbolLength == 16, "symbol field must be 16 bytes");
static_assert(kOffsetSymbol + kSymbolLength == kMessageSize,
              "symbol must end at message boundary");
static_assert(kOffsetHeaderReserved + kSizeHeaderReserved == kOffsetTimestamp,
              "header layout gap before timestamp");
static_assert(kOffsetTimestamp + kSizeTimestamp == kOffsetCommandType,
              "timestamp must be immediately before command_type");
static_assert(kOffsetAlignReserved + kSizeAlignReserved == kOffsetOrderId,
              "alignment gap before order_id");
static_assert(kOffsetQuantity + kSizeQuantity == kOffsetSymbol,
              "quantity must be immediately before symbol");

using BinaryMessage = std::array<std::byte, kMessageSize>;

/// Encodes one OrderCommand as a fixed 64-byte OBK1 v1 message (little-endian).
/// @param timestamp Wire-only metadata (nanoseconds or application ticks).
/// @throws std::invalid_argument on unsupported enums or symbol longer than 16 bytes.
BinaryMessage encode_order_command(const matching_engine::OrderCommand& command,
                                   uint64_t timestamp = 0);

struct DecodedOrderCommand {
    matching_engine::OrderCommand command;
    uint64_t timestamp{};
};

/// Decodes one 64-byte OBK1 v1 message into OrderCommand + wire timestamp.
/// @throws std::invalid_argument on invalid header, enums, reserved bytes, or symbol.
DecodedOrderCommand decode_order_command(const BinaryMessage& message);

}  // namespace protocol
