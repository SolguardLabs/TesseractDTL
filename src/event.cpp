#include "event.hpp"

namespace tdtl {

std::string eventTypeName(EventType type) {
    switch (type) {
    case EventType::AssetRegistered:
        return "asset_registered";
    case EventType::DomainRegistered:
        return "domain_registered";
    case EventType::OperatorRegistered:
        return "operator_registered";
    case EventType::WindowOpened:
        return "window_opened";
    case EventType::VaultRegistered:
        return "vault_registered";
    case EventType::DepositAccepted:
        return "deposit_accepted";
    case EventType::RouteCreated:
        return "route_created";
    case EventType::PlanReduced:
        return "plan_reduced";
    case EventType::SettlementExecuted:
        return "settlement_executed";
    case EventType::ReconciliationFinished:
        return "reconciliation_finished";
    case EventType::SurplusSwept:
        return "surplus_swept";
    }
    return "unknown";
}

std::string Event::field(const std::string& key) const {
    auto it = fields.find(key);
    if (it == fields.end()) {
        return "";
    }
    return it->second;
}

void EventLog::push(Event event) {
    events_.push_back(std::move(event));
}

void EventLog::push(EventType type, EpochId epoch, std::string subject) {
    Event event;
    event.type = type;
    event.epoch = epoch;
    event.subject = std::move(subject);
    events_.push_back(std::move(event));
}

void EventLog::pushAmount(EventType type, EpochId epoch, std::string subject, Amount amount) {
    Event event;
    event.type = type;
    event.epoch = epoch;
    event.subject = std::move(subject);
    event.fields["amount"] = amount.str();
    events_.push_back(std::move(event));
}

const std::vector<Event>& EventLog::all() const {
    return events_;
}

std::vector<Event> EventLog::byType(EventType type) const {
    std::vector<Event> out;
    for (const auto& event : events_) {
        if (event.type == type) {
            out.push_back(event);
        }
    }
    return out;
}

std::vector<Event> EventLog::byEpoch(EpochId epoch) const {
    std::vector<Event> out;
    for (const auto& event : events_) {
        if (event.epoch == epoch) {
            out.push_back(event);
        }
    }
    return out;
}

std::size_t EventLog::size() const {
    return events_.size();
}

bool EventLog::empty() const {
    return events_.empty();
}

void EventLog::clear() {
    events_.clear();
}

} // namespace tdtl
