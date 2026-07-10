#pragma once

#include <stdexcept>
#include <string>

namespace tdtl {

class ProtocolError : public std::runtime_error {
public:
    explicit ProtocolError(const std::string& message);
};

class ValidationError : public ProtocolError {
public:
    explicit ValidationError(const std::string& message);
};

class LedgerError : public ProtocolError {
public:
    explicit LedgerError(const std::string& message);
};

class ScenarioError : public ProtocolError {
public:
    explicit ScenarioError(const std::string& message);
};

} // namespace tdtl
