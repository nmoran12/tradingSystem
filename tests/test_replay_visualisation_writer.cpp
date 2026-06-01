#include "matching_engine/MatchingEngine.hpp"
#include "viz/ReplayVisualisationWriter.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

using namespace matching_engine;
using namespace market_data;
using namespace viz;

namespace {

OrderCommand make_new(uint64_t order_id, Side side, int64_t price, uint64_t quantity) {
    OrderCommand command;
    command.type = OrderCommandType::NewOrder;
    command.order_id = order_id;
    command.side = side;
    command.order_type = OrderType::Limit;
    command.price = price;
    command.quantity = quantity;
    command.symbol = "AAPL";
    return command;
}

void expect_field_present(const std::string& line, const std::string& field) {
    EXPECT_NE(line.find(field), std::string::npos) << "missing field " << field;
}

}  // namespace

TEST(ReplayVisualisationWriterTest, FormatRecordMatchesNdjsonShape) {
    MatchingEngine engine;
    std::vector<EngineEvent> scratch;
    scratch.reserve(4);

    const auto command = make_new(1, Side::SELL, 100, 10);
    engine.process_into(command, scratch);

    const std::string record =
        ReplayVisualisationWriter::format_record(0, command, scratch, engine.book());

    expect_field_present(record, "\"schemaVersion\":1");
    expect_field_present(record, "\"index\":0");
    expect_field_present(record, "\"commandType\":\"new\"");
    expect_field_present(record, "\"trades\":[]");
    EXPECT_EQ(record.front(), '{');
    EXPECT_EQ(record.back(), '}');
    EXPECT_EQ(record.find('\n'), std::string::npos);
}

TEST(ReplayVisualisationWriterTest, WriteNdjsonLineEndsWithNewline) {
    MatchingEngine engine;
    std::vector<EngineEvent> scratch;
    const auto command = make_new(1, Side::BUY, 100, 5);
    engine.process_into(command, scratch);

    std::ostringstream out;
    ReplayVisualisationWriter::write_ndjson_line(out, 3, command, scratch, engine.book());

    const std::string line = out.str();
    ASSERT_FALSE(line.empty());
    EXPECT_EQ(line.back(), '\n');
    expect_field_present(line, "\"index\":3");
}

TEST(ReplayVisualisationWriterTest, FormatSseFrameUsesDataPrefix) {
    const std::string frame = ReplayVisualisationWriter::format_sse_frame("{\"schemaVersion\":1}");
    EXPECT_EQ(frame, "data: {\"schemaVersion\":1}\n\n");
}

TEST(ReplayVisualisationWriterTest, FormatRecordIncludesTradeWhenOrdersCross) {
    MatchingEngine engine;
    std::vector<EngineEvent> scratch;
    scratch.reserve(4);

    engine.process_into(make_new(1, Side::SELL, 100, 10), scratch);
    const auto buy = make_new(2, Side::BUY, 101, 10);
    engine.process_into(buy, scratch);

    const std::string record =
        ReplayVisualisationWriter::format_record(1, buy, scratch, engine.book());

    expect_field_present(record, "\"schemaVersion\":1");
    expect_field_present(record, "\"price\":100");
    expect_field_present(record, "\"quantity\":10");
    expect_field_present(record, "\"aggressiveOrderId\":2");
    expect_field_present(record, "\"restingOrderId\":1");
}
