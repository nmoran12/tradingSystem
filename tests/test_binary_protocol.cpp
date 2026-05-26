#include "matching_engine/OrderCommand.hpp"
#include "protocol/BinaryProtocol.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

using namespace market_data;
using namespace matching_engine;
using namespace protocol;

namespace {

uint8_t byte_at(const BinaryMessage& message, std::size_t offset) {
    return static_cast<uint8_t>(message[offset]);
}

uint32_t read_u32_le(const BinaryMessage& message, std::size_t offset) {
    return static_cast<uint32_t>(byte_at(message, offset)) |
           (static_cast<uint32_t>(byte_at(message, offset + 1)) << 8) |
           (static_cast<uint32_t>(byte_at(message, offset + 2)) << 16) |
           (static_cast<uint32_t>(byte_at(message, offset + 3)) << 24);
}

uint64_t read_u64_le(const BinaryMessage& message, std::size_t offset) {
    uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(byte_at(message, offset + i)) << (8 * i);
    }
    return value;
}

int64_t read_i64_le(const BinaryMessage& message, std::size_t offset) {
    return static_cast<int64_t>(read_u64_le(message, offset));
}

OrderCommand make_command(OrderCommandType type, uint64_t order_id, Side side,
                          OrderType order_type, int64_t price, uint64_t quantity,
                          const std::string& symbol) {
    OrderCommand command;
    command.type = type;
    command.order_id = order_id;
    command.side = side;
    command.order_type = order_type;
    command.price = price;
    command.quantity = quantity;
    command.symbol = symbol;
    return command;
}

void expect_reserved_zero(const BinaryMessage& message) {
    EXPECT_EQ(byte_at(message, kOffsetHeaderReserved), 0u);
    EXPECT_EQ(byte_at(message, kOffsetHeaderReserved + 1), 0u);
    EXPECT_EQ(byte_at(message, kOffsetPayloadReserved), 0u);
    for (std::size_t i = 0; i < kSizeAlignReserved; ++i) {
        EXPECT_EQ(byte_at(message, kOffsetAlignReserved + i), 0u);
    }
}

BinaryMessage with_byte(const BinaryMessage& message, std::size_t offset, uint8_t value) {
    auto copy = message;
    copy[offset] = static_cast<std::byte>(value);
    return copy;
}

void expect_commands_equal(const OrderCommand& lhs, const OrderCommand& rhs) {
    EXPECT_EQ(lhs.type, rhs.type);
    EXPECT_EQ(lhs.order_id, rhs.order_id);
    EXPECT_EQ(lhs.side, rhs.side);
    EXPECT_EQ(lhs.order_type, rhs.order_type);
    EXPECT_EQ(lhs.price, rhs.price);
    EXPECT_EQ(lhs.quantity, rhs.quantity);
    EXPECT_EQ(lhs.symbol, rhs.symbol);
}

}  // namespace

TEST(BinaryProtocolEncoderTest, EncodesMessageToExactly64Bytes) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 10050, 10, "AAPL"));
    EXPECT_EQ(message.size(), kMessageSize);
}

TEST(BinaryProtocolEncoderTest, EncodedMagicBytesMatchObk1LittleEndian) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    EXPECT_EQ(byte_at(message, 0), 0x4Fu);
    EXPECT_EQ(byte_at(message, 1), 0x42u);
    EXPECT_EQ(byte_at(message, 2), 0x4Bu);
    EXPECT_EQ(byte_at(message, 3), 0x31u);
    EXPECT_EQ(read_u32_le(message, kOffsetMagic), kMagic);
}

TEST(BinaryProtocolEncoderTest, EncodedVersionIsOne) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    EXPECT_EQ(byte_at(message, kOffsetVersion), kProtocolVersion);
}

TEST(BinaryProtocolEncoderTest, EncodedWireMessageTypeIsOrderCommand) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    EXPECT_EQ(byte_at(message, kOffsetWireMessageType), kWireMessageTypeOrderCommand);
}

TEST(BinaryProtocolEncoderTest, EncodesNewOrderCommandType) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    EXPECT_EQ(byte_at(message, kOffsetCommandType), kCommandTypeNewOrder);
}

TEST(BinaryProtocolEncoderTest, EncodesCancelOrderCommandType) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::CancelOrder, 2, Side::UNKNOWN, OrderType::Limit, 0, 0, "X"));
    EXPECT_EQ(byte_at(message, kOffsetCommandType), kCommandTypeCancelOrder);
}

TEST(BinaryProtocolEncoderTest, EncodesModifyOrderCommandType) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::ModifyOrder, 3, Side::SELL, OrderType::Limit, 10050, 75, "X"));
    EXPECT_EQ(byte_at(message, kOffsetCommandType), kCommandTypeModifyOrder);
}

TEST(BinaryProtocolEncoderTest, EncodesBuySide) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    EXPECT_EQ(byte_at(message, kOffsetSide), kSideBuy);
}

TEST(BinaryProtocolEncoderTest, EncodesSellSide) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 0, 1, "X"));
    EXPECT_EQ(byte_at(message, kOffsetSide), kSideSell);
}

TEST(BinaryProtocolEncoderTest, EncodesUnknownSideForCancel) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::CancelOrder, 9, Side::UNKNOWN, OrderType::Limit, 0, 0, "X"));
    EXPECT_EQ(byte_at(message, kOffsetSide), kSideUnknown);
}

TEST(BinaryProtocolEncoderTest, EncodesLimitAndMarketOrderTypes) {
    const auto limit = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 100, 1, "X"));
    const auto market = encode_order_command(make_command(
        OrderCommandType::NewOrder, 2, Side::BUY, OrderType::Market, 0, 1, "X"));
    EXPECT_EQ(byte_at(limit, kOffsetOrderType), kOrderTypeLimit);
    EXPECT_EQ(byte_at(market, kOffsetOrderType), kOrderTypeMarket);
}

TEST(BinaryProtocolEncoderTest, EncodesOrderIdPriceQuantityTimestampLittleEndian) {
    const auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 0x0102030405060708ULL, Side::BUY,
                     OrderType::Limit, 0x0807060504030201LL, 0x0F0E0D0C0B0A0908ULL, "Z"),
        0x0102030405060708ULL);

    EXPECT_EQ(read_u64_le(message, kOffsetTimestamp), 0x0102030405060708ULL);
    EXPECT_EQ(read_u64_le(message, kOffsetOrderId), 0x0102030405060708ULL);
    EXPECT_EQ(read_i64_le(message, kOffsetPrice), 0x0807060504030201LL);
    EXPECT_EQ(read_u64_le(message, kOffsetQuantity), 0x0F0E0D0C0B0A0908ULL);
}

TEST(BinaryProtocolEncoderTest, EncodesShortSymbolWithNullPadding) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "AAPL"));
    EXPECT_EQ(byte_at(message, kOffsetSymbol + 0), 'A');
    EXPECT_EQ(byte_at(message, kOffsetSymbol + 1), 'A');
    EXPECT_EQ(byte_at(message, kOffsetSymbol + 2), 'P');
    EXPECT_EQ(byte_at(message, kOffsetSymbol + 3), 'L');
    EXPECT_EQ(byte_at(message, kOffsetSymbol + 4), 0u);
    EXPECT_EQ(byte_at(message, kOffsetSymbol + 15), 0u);
}

TEST(BinaryProtocolEncoderTest, EncodesExactly16ByteSymbol) {
    const std::string symbol = "ABCDEFGHIJKLMNOP";
    ASSERT_EQ(symbol.size(), kSymbolLength);
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, symbol));
    for (std::size_t i = 0; i < kSymbolLength; ++i) {
        EXPECT_EQ(byte_at(message, kOffsetSymbol + i), static_cast<uint8_t>(symbol[i]));
    }
}

TEST(BinaryProtocolEncoderTest, RejectsSymbolLongerThan16Bytes) {
    EXPECT_THROW(encode_order_command(make_command(
                     OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1,
                     "ABCDEFGHIJKLMNOPQ")),
                 std::invalid_argument);
}

TEST(BinaryProtocolEncoderTest, RejectsUnknownSideForNewOrder) {
    EXPECT_THROW(encode_order_command(make_command(
                     OrderCommandType::NewOrder, 1, Side::UNKNOWN, OrderType::Limit, 0, 1, "X")),
                 std::invalid_argument);
}

TEST(BinaryProtocolEncoderTest, ReservedBytesAreZero) {
    const auto message = encode_order_command(make_command(
        OrderCommandType::ModifyOrder, 5, Side::SELL, OrderType::Market, 10055, 25, "SYM"),
        999);
    expect_reserved_zero(message);
}

TEST(BinaryProtocolEncoderTest, SnapshotMatchesDocumentedExample) {
    const auto command = make_command(OrderCommandType::NewOrder, 1, Side::SELL,
                                      OrderType::Limit, 10055, 100, "AAPL");
    const auto message = encode_order_command(command, 1001);

    const std::array<uint8_t, kMessageSize> expected = {
        0x4F, 0x42, 0x4B, 0x31, 0x01, 0x01, 0x00, 0x00, 0xE9, 0x03, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x47, 0x27, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x64, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x41, 0x41, 0x50, 0x4C, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    for (std::size_t i = 0; i < kMessageSize; ++i) {
        EXPECT_EQ(byte_at(message, i), expected[i]) << "offset " << i;
    }
}

TEST(BinaryProtocolDecoderTest, RejectsInvalidMagic) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, 0, 0x00);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsInvalidVersion) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetVersion, 0x02);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsInvalidWireMessageType) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetWireMessageType, 0x02);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsNonZeroHeaderReservedBytes) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetHeaderReserved, 0x01);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsInvalidCommandType) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetCommandType, 0x04);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsInvalidSide) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetSide, 0x03);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsInvalidOrderType) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetOrderType, 0x03);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsNonZeroPayloadReservedBytes) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetPayloadReserved, 0x01);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsNonZeroAlignReservedBytes) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    message = with_byte(message, kOffsetAlignReserved, 0x01);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, DecodesTimestampCorrectly) {
    constexpr uint64_t timestamp = 1001;
    const auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::SELL, OrderType::Limit, 10055, 100, "AAPL"),
        timestamp);
    const auto decoded = decode_order_command(message);
    EXPECT_EQ(decoded.timestamp, timestamp);
}

TEST(BinaryProtocolDecoderTest, DecodesOrderIdPriceAndQuantityCorrectly) {
    const auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 42, Side::BUY, OrderType::Limit, -500, 999, "Z"),
        0);
    const auto decoded = decode_order_command(message);
    EXPECT_EQ(decoded.command.order_id, 42u);
    EXPECT_EQ(decoded.command.price, -500);
    EXPECT_EQ(decoded.command.quantity, 999u);
}

TEST(BinaryProtocolDecoderTest, DecodesShortNulPaddedSymbol) {
    const auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "AAPL"));
    const auto decoded = decode_order_command(message);
    EXPECT_EQ(decoded.command.symbol, "AAPL");
}

TEST(BinaryProtocolDecoderTest, DecodesExactly16ByteSymbol) {
    const std::string symbol = "ABCDEFGHIJKLMNOP";
    const auto message = encode_order_command(make_command(
        OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, symbol));
    const auto decoded = decode_order_command(message);
    EXPECT_EQ(decoded.command.symbol, symbol);
}

TEST(BinaryProtocolDecoderTest, RejectsEmptySymbolForNewOrder) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "X"));
    for (std::size_t i = 0; i < kSymbolLength; ++i) {
        message[kOffsetSymbol + i] = std::byte{0};
    }
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, DecodesSpacePaddedSymbol) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "AAPL"));
    for (std::size_t i = 4; i < kSymbolLength; ++i) {
        message[kOffsetSymbol + i] = std::byte{0x20};
    }
    const auto decoded = decode_order_command(message);
    EXPECT_EQ(decoded.command.symbol, "AAPL");
}

TEST(BinaryProtocolDecoderTest, RejectsGarbageAfterNulInSymbolField) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "AAPL"));
    message[kOffsetSymbol + 4] = std::byte{0};
    message[kOffsetSymbol + 5] = std::byte{'X'};
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, RejectsNonPaddingByteInSymbolPaddingRegion) {
    auto message = encode_order_command(
        make_command(OrderCommandType::NewOrder, 1, Side::BUY, OrderType::Limit, 0, 1, "AAPL"));
    message[kOffsetSymbol + 7] = std::byte{0x41};
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolDecoderTest, CancelEmptySymbolMapsToDefault) {
    auto message = encode_order_command(make_command(
        OrderCommandType::CancelOrder, 9, Side::UNKNOWN, OrderType::Limit, 0, 0, "X"));
    for (std::size_t i = 0; i < kSymbolLength; ++i) {
        message[kOffsetSymbol + i] = std::byte{0};
    }
    const auto decoded = decode_order_command(message);
    EXPECT_EQ(decoded.command.symbol, "DEFAULT");
}

TEST(BinaryProtocolDecoderTest, CancelAllSpaceSymbolMapsToDefault) {
    auto message = encode_order_command(make_command(
        OrderCommandType::CancelOrder, 9, Side::UNKNOWN, OrderType::Limit, 0, 0, "X"));
    for (std::size_t i = 0; i < kSymbolLength; ++i) {
        message[kOffsetSymbol + i] = std::byte{0x20};
    }
    const auto decoded = decode_order_command(message);
    EXPECT_EQ(decoded.command.symbol, "DEFAULT");
}

TEST(BinaryProtocolDecoderTest, RejectsUnknownSideForNewOrder) {
    auto message = encode_order_command(
        make_command(OrderCommandType::CancelOrder, 1, Side::UNKNOWN, OrderType::Limit, 0, 0, "X"));
    message = with_byte(message, kOffsetCommandType, kCommandTypeNewOrder);
    EXPECT_THROW(decode_order_command(message), std::invalid_argument);
}

TEST(BinaryProtocolRoundTripTest, NewOrderBuyLimit) {
    const auto original = make_command(OrderCommandType::NewOrder, 7, Side::BUY, OrderType::Limit,
                                       10050, 250, "MSFT");
    constexpr uint64_t timestamp = 12345;
    const auto decoded = decode_order_command(encode_order_command(original, timestamp));
    expect_commands_equal(decoded.command, original);
    EXPECT_EQ(decoded.timestamp, timestamp);
}

TEST(BinaryProtocolRoundTripTest, NewOrderSellLimit) {
    const auto original = make_command(OrderCommandType::NewOrder, 8, Side::SELL, OrderType::Limit,
                                       10055, 100, "AAPL");
    const auto decoded = decode_order_command(encode_order_command(original, 99));
    expect_commands_equal(decoded.command, original);
    EXPECT_EQ(decoded.timestamp, 99u);
}

TEST(BinaryProtocolRoundTripTest, CancelOrderWithUnknownSide) {
    const auto original = make_command(OrderCommandType::CancelOrder, 9, Side::UNKNOWN,
                                       OrderType::Limit, 0, 0, "X");
    const auto decoded = decode_order_command(encode_order_command(original, 500));
    expect_commands_equal(decoded.command, original);
    EXPECT_EQ(decoded.timestamp, 500u);
}

TEST(BinaryProtocolRoundTripTest, ModifyOrder) {
    const auto original = make_command(OrderCommandType::ModifyOrder, 10, Side::SELL,
                                       OrderType::Limit, 10050, 75, "GOOG");
    const auto decoded = decode_order_command(encode_order_command(original, 777));
    expect_commands_equal(decoded.command, original);
    EXPECT_EQ(decoded.timestamp, 777u);
}

TEST(BinaryProtocolRoundTripTest, NewOrderMarket) {
    const auto original = make_command(OrderCommandType::NewOrder, 11, Side::BUY, OrderType::Market,
                                       0, 25, "SYM");
    const auto decoded = decode_order_command(encode_order_command(original, 1));
    expect_commands_equal(decoded.command, original);
}
