#pragma once

#include "scenario.hpp"

#include <iosfwd>
#include <sstream>
#include <string>

namespace tdtl {

class JsonStream {
public:
    explicit JsonStream(std::ostream& out);

    void objectBegin();
    void objectEnd();
    void arrayBegin();
    void arrayEnd();
    void key(const std::string& key);
    void comma();
    void string(const std::string& value);
    void number(std::int64_t value);
    void boolean(bool value);
    void raw(const std::string& value);

private:
    static std::string escape(const std::string& value);

    std::ostream& out_;
};

class ReportWriter {
public:
    static void writeJson(std::ostream& out, const ScenarioState& state, bool includeEvents);
    static void writeText(std::ostream& out, const ScenarioState& state);
    static void writeScenarioList(std::ostream& out, const std::vector<ScenarioInfo>& scenarios, bool json);

private:
    static void writeSummary(JsonStream& json, const ScenarioState& state);
    static void writeRegistry(JsonStream& json, const ScenarioState& state);
    static void writeAccounts(JsonStream& json, const ScenarioState& state);
    static void writeVaults(JsonStream& json, const ScenarioState& state);
    static void writeOrders(JsonStream& json, const ScenarioState& state);
    static void writeRoutes(JsonStream& json, const ScenarioState& state);
    static void writeEpochs(JsonStream& json, const ScenarioState& state);
    static void writeReconciliation(JsonStream& json, const ScenarioState& state);
    static void writeEvents(JsonStream& json, const ScenarioState& state);
};

} // namespace tdtl
