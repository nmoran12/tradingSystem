#include "market_data/MarketDataParser.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>

using namespace market_data;

TEST(MarketDataParserTest, ParsesValidAddEvent) {
    const auto event = MarketDataParser::parse_line("1001,ADD,AAPL,BUY,1,10050,200");
    EXPECT_EQ(event.timestamp, 1001u);
    EXPECT_EQ(event.type, EventType::ADD);
    EXPECT_EQ(event.symbol, "AAPL");
    EXPECT_EQ(event.side, Side::BUY);
    EXPECT_EQ(event.order_id, 1u);
    EXPECT_EQ(event.price, 10050);
    EXPECT_EQ(event.quantity, 200u);
}

TEST(MarketDataParserTest, ParsesValidCancelEvent) {
    const auto event = MarketDataParser::parse_line("1003,CANCEL,AAPL,BUY,1,10050,0");
    EXPECT_EQ(event.type, EventType::CANCEL);
    EXPECT_EQ(event.side, Side::BUY);
    EXPECT_EQ(event.quantity, 0u);
}

TEST(MarketDataParserTest, ParsesValidExecuteEvent) {
    const auto event = MarketDataParser::parse_line("1004,EXECUTE,AAPL,SELL,2,10055,50");
    EXPECT_EQ(event.type, EventType::EXECUTE);
    EXPECT_EQ(event.side, Side::SELL);
    EXPECT_EQ(event.quantity, 50u);
}

TEST(MarketDataParserTest, RejectsMalformedRows) {
    EXPECT_THROW(MarketDataParser::parse_line("1001,ADD,AAPL,BUY,1,10050"), std::invalid_argument);
    EXPECT_THROW(MarketDataParser::parse_line("1001,ADD,AAPL,LEFT,1,10050,200"),
                 std::invalid_argument);
    EXPECT_THROW(MarketDataParser::parse_line("bad,ADD,AAPL,BUY,1,10050,200"),
                 std::invalid_argument);
}

TEST(MarketDataParserTest, SkipsHeaderRow) {
    std::istringstream input(
        "timestamp,type,symbol,side,order_id,price,quantity\n"
        "1001,ADD,AAPL,BUY,1,10050,200\n");
    const auto events = MarketDataParser::parse_stream(input);
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0].order_id, 1u);
}
