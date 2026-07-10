#pragma once

#include "amount.hpp"
#include "identifiers.hpp"

#include <map>
#include <string>
#include <vector>

namespace tdtl {

enum class EventType {
    AssetRegistered,
    DomainRegistered,
    OperatorRegistered,
    WindowOpened,
    VaultRegistered,
    DepositAccepted,
    RouteCreated,
    PlanReduced,
    SettlementExecuted,
    ReconciliationFinished,
    SurplusSwept
};

std::string eventTypeName(EventType type);

struct Event {
    EventType type{EventType::AssetRegistered};
    EpochId epoch;
    std::string subject;
    std::map<std::string, std::string> fields;

    std::string field(const std::string& key) const;
};

class EventLog {
public:
    void push(Event event);
    void push(EventType type, EpochId epoch, std::string subject);
    void pushAmount(EventType type, EpochId epoch, std::string subject, Amount amount);
    const std::vector<Event>& all() const;
    std::vector<Event> byType(EventType type) const;
    std::vector<Event> byEpoch(EpochId epoch) const;
    std::size_t size() const;
    bool empty() const;
    void clear();

private:
    std::vector<Event> events_;
};

} // namespace tdtl
