#pragma once

#include "matching_engine/EngineEvent.hpp"
#include "matching_engine/OrderCommand.hpp"
#include "order_book/OrderBook.hpp"

#include <cstddef>
#include <ostream>
#include <string>
#include <vector>

namespace viz {

// Formats schemaVersion 1 replay visualisation records (same shape as 7A NDJSON export).
class ReplayVisualisationWriter {
public:
    static void write_ndjson_line(std::ostream& out, std::size_t index,
                                  const matching_engine::OrderCommand& command,
                                  const std::vector<matching_engine::EngineEvent>& events,
                                  const order_book::OrderBook& book);

    static std::string format_record(std::size_t index,
                                     const matching_engine::OrderCommand& command,
                                     const std::vector<matching_engine::EngineEvent>& events,
                                     const order_book::OrderBook& book);

    static std::string format_sse_frame(const std::string& json_record);
};

}  // namespace viz
