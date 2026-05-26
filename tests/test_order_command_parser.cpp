#include "market_data/OrderCommandParser.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <stdexcept>

using namespace market_data;
using namespace matching_engine;

TEST(OrderCommandParserTest, ParsesValidNewLimitCommand) {
    const auto command = OrderCommandParser::parse_line("NEW,1,SELL,LIMIT,10055,100");
    EXPECT_EQ(command.type, OrderCommandType::NewOrder);
    EXPECT_EQ(command.order_id, 1u);
    EXPECT_EQ(command.side, Side::SELL);
    EXPECT_EQ(command.order_type, OrderType::Limit);
    EXPECT_EQ(command.price, 10055);
    EXPECT_EQ(command.quantity, 100u);
}

TEST(OrderCommandParserTest, ParsesValidNewMarketCommand) {
    const auto command = OrderCommandParser::parse_line("NEW,4,BUY,MARKET,0,25");
    EXPECT_EQ(command.order_type, OrderType::Market);
    EXPECT_EQ(command.price, 0);
    EXPECT_EQ(command.quantity, 25u);
}

TEST(OrderCommandParserTest, ParsesValidCancelCommand) {
    const auto command = OrderCommandParser::parse_line("CANCEL,2,BUY,LIMIT,0,0");
    EXPECT_EQ(command.type, OrderCommandType::CancelOrder);
    EXPECT_EQ(command.order_id, 2u);
}

TEST(OrderCommandParserTest, ParsesValidModifyCommand) {
    const auto command = OrderCommandParser::parse_line("MODIFY,1,SELL,LIMIT,10050,75");
    EXPECT_EQ(command.type, OrderCommandType::ModifyOrder);
    EXPECT_EQ(command.price, 10050);
    EXPECT_EQ(command.quantity, 75u);
}

TEST(OrderCommandParserTest, RejectsInvalidType) {
    EXPECT_THROW(OrderCommandParser::parse_line("HOLD,1,BUY,LIMIT,100,10"), std::invalid_argument);
}

TEST(OrderCommandParserTest, RejectsInvalidSide) {
    EXPECT_THROW(OrderCommandParser::parse_line("NEW,1,LEFT,LIMIT,100,10"), std::invalid_argument);
}

TEST(OrderCommandParserTest, RejectsInvalidOrderType) {
    EXPECT_THROW(OrderCommandParser::parse_line("NEW,1,BUY,STOP,100,10"), std::invalid_argument);
}

TEST(OrderCommandParserTest, RejectsInvalidNumericField) {
    EXPECT_THROW(OrderCommandParser::parse_line("NEW,bad,BUY,LIMIT,100,10"), std::invalid_argument);
}

TEST(OrderCommandParserTest, RejectsMissingColumns) {
    EXPECT_THROW(OrderCommandParser::parse_line("NEW,1,BUY,LIMIT,100"), std::invalid_argument);
}

TEST(OrderCommandParserTest, SkipsHeaderRow) {
    std::istringstream input(
        "type,order_id,side,order_type,price,quantity\n"
        "NEW,1,BUY,LIMIT,10050,10\n");
    const auto commands = OrderCommandParser::parse_stream(input);
    ASSERT_EQ(commands.size(), 1u);
    EXPECT_EQ(commands[0].order_id, 1u);
}
