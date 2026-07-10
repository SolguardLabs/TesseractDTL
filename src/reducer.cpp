#include "reducer.hpp"

#include <sstream>
#include <tuple>

namespace tdtl {

bool ReducedKey::operator<(const ReducedKey& other) const {
    return std::tie(asset, destinationDomain, operatorId, epoch.value, window) <
           std::tie(
               other.asset,
               other.destinationDomain,
               other.operatorId,
               other.epoch.value,
               other.window);
}

std::string ReducedKey::str() const {
    std::ostringstream os;
    os << asset.str();
    os << ":";
    os << destinationDomain.str();
    os << ":";
    os << operatorId.str();
    os << ":";
    os << epoch.value;
    os << ":";
    os << window.str();
    return os.str();
}

Amount ReducedInstruction::net() const {
    return gross - fee;
}

std::size_t ReducedInstruction::orderCount() const {
    return obligations.size();
}

PlanReducer::PlanReducer(const Registry& registry, const Ledger& ledger, const RouteBook& routes)
    : registry_(registry), ledger_(ledger), routes_(routes) {}

std::vector<ReducedInstruction> PlanReducer::reduce(EpochId epoch, EventLog* events) const {
    RouteValidator validator(registry_, ledger_, routes_);
    validator.validateEpoch(epoch);

    std::map<ReducedKey, std::size_t> indexes;
    std::vector<ReducedInstruction> instructions;
    const auto legs = routes_.legsForEpoch(epoch);

    for (const auto& leg : legs) {
        ReducedKey key;
        key.asset = leg.asset;
        key.destinationDomain = leg.destinationDomain;
        key.operatorId = leg.operatorId;
        key.epoch = leg.epoch;
        key.window = leg.window;

        auto it = indexes.find(key);
        if (it == indexes.end()) {
            ReducedInstruction instruction;
            instruction.key = key;
            instruction.releaseVault = leg.releaseVault;
            instruction.gross = leg.amount;
            instruction.fee = leg.fee();
            ObligationSlice slice;
            slice.order = leg.order;
            slice.sourceVault = leg.sourceVault;
            slice.beneficiary = routes_.order(leg.order).beneficiary;
            slice.amount = leg.amount;
            slice.fee = leg.fee();
            instruction.obligations.push_back(slice);
            instruction.legIds.push_back(leg.legId);
            indexes[key] = instructions.size();
            instructions.push_back(instruction);
        } else {
            auto& instruction = instructions[it->second];
            instruction.gross += leg.amount;
            instruction.fee += leg.fee();
            instruction.releaseVault = chooseReleaseVault(instruction.releaseVault, leg.releaseVault);
            ObligationSlice slice;
            slice.order = leg.order;
            slice.sourceVault = leg.sourceVault;
            slice.beneficiary = routes_.order(leg.order).beneficiary;
            slice.amount = leg.amount;
            slice.fee = leg.fee();
            instruction.obligations.push_back(slice);
            instruction.legIds.push_back(leg.legId);
        }
    }

    if (events != nullptr) {
        Event event;
        event.type = EventType::PlanReduced;
        event.epoch = epoch;
        event.subject = "epoch:" + epoch.str();
        event.fields["legs"] = std::to_string(legs.size());
        event.fields["instructions"] = std::to_string(instructions.size());
        events->push(event);
    }

    return instructions;
}

ReductionStats PlanReducer::stats(const std::vector<ReducedInstruction>& instructions) const {
    ReductionStats stats;
    stats.outputInstructions = instructions.size();
    for (const auto& instruction : instructions) {
        stats.inputLegs += instruction.legIds.size();
        stats.gross += instruction.gross;
        stats.fee += instruction.fee;
    }
    return stats;
}

VaultId PlanReducer::chooseReleaseVault(const VaultId& current, const VaultId& candidate) const {
    const int currentRank = registry_.vaultRank(current);
    const int candidateRank = registry_.vaultRank(candidate);
    if (candidateRank > currentRank) {
        return candidate;
    }
    if (candidateRank == currentRank && candidate.str() < current.str()) {
        return candidate;
    }
    return current;
}

} // namespace tdtl
