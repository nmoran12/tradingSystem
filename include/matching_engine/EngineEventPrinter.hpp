#pragma once

#include "matching_engine/EngineEvent.hpp"

#include <iosfwd>
#include <vector>

namespace matching_engine {

void print_engine_event(std::ostream& out, const EngineEvent& event);
void print_engine_events(std::ostream& out, const std::vector<EngineEvent>& events);

const char* to_string(EngineEventType type);

}  // namespace matching_engine
