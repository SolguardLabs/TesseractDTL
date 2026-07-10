#include "errors.hpp"

namespace tdtl {

ProtocolError::ProtocolError(const std::string& message) : std::runtime_error(message) {}

ValidationError::ValidationError(const std::string& message) : ProtocolError(message) {}

LedgerError::LedgerError(const std::string& message) : ProtocolError(message) {}

ScenarioError::ScenarioError(const std::string& message) : ProtocolError(message) {}

} // namespace tdtl
