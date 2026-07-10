#include "settlement.hpp"

#include "errors.hpp"

namespace tdtl {

SettlementEngine::SettlementEngine(
    Registry& registry,
    Ledger& ledger,
    RouteBook& routes,
    EventLog& events)
    : registry_(registry), ledger_(ledger), routes_(routes), events_(events) {}

EpochReceipt SettlementEngine::settleEpoch(EpochId epoch) {
    if (isSettled(epoch)) {
        throw ValidationError("epoch already settled " + epoch.str());
    }
    PlanReducer reducer(registry_, ledger_, routes_);
    const auto instructions = reducer.reduce(epoch, &events_);

    EpochReceipt receipt;
    receipt.epoch = epoch;
    receipt.settled = true;

    for (const auto& instruction : instructions) {
        auto applied = applyInstruction(instruction);
        receipt.gross += applied.gross;
        receipt.fee += applied.fee;
        receipt.net += applied.net;
        receipt.instructions.push_back(applied);
        markOrdersSettled(instruction);
    }

    settledEpochs_.insert(epoch.value);
    receipts_.push_back(receipt);

    Event event;
    event.type = EventType::SettlementExecuted;
    event.epoch = epoch;
    event.subject = "epoch:" + epoch.str();
    event.fields["instructions"] = std::to_string(receipt.instructions.size());
    event.fields["gross"] = receipt.gross.str();
    event.fields["fee"] = receipt.fee.str();
    event.fields["net"] = receipt.net.str();
    events_.push(event);

    return receipt;
}

std::vector<EpochReceipt> SettlementEngine::settleAll() {
    std::vector<EpochReceipt> out;
    for (const auto& epoch : routes_.epochs()) {
        if (!isSettled(epoch)) {
            out.push_back(settleEpoch(epoch));
        }
    }
    return out;
}

std::vector<EpochReceipt> SettlementEngine::settleUntil(EpochId maxEpoch) {
    std::vector<EpochReceipt> out;
    for (const auto& epoch : routes_.epochs()) {
        if (epoch <= maxEpoch && !isSettled(epoch)) {
            out.push_back(settleEpoch(epoch));
        }
    }
    return out;
}

bool SettlementEngine::isSettled(EpochId epoch) const {
    return settledEpochs_.find(epoch.value) != settledEpochs_.end();
}

const std::vector<EpochReceipt>& SettlementEngine::receipts() const {
    return receipts_;
}

InstructionReceipt SettlementEngine::applyInstruction(const ReducedInstruction& instruction) {
    const auto& op = registry_.operatorInfo(instruction.key.operatorId);
    const AssetId asset = instruction.key.asset;
    ledger_.releaseFromVault(instruction.releaseVault, instruction.gross, instruction.fee);

    for (const auto& slice : instruction.obligations) {
        ledger_.closeLiability(slice.order, slice.sourceVault, slice.amount);
        ledger_.credit(slice.beneficiary, asset, slice.amount - slice.fee);
    }

    if (instruction.fee > Amount::zero()) {
        ledger_.credit(op.feeAccount, asset, instruction.fee);
    }

    InstructionReceipt receipt;
    receipt.key = instruction.key.str();
    receipt.releaseVault = instruction.releaseVault;
    receipt.asset = asset;
    receipt.operatorId = instruction.key.operatorId;
    receipt.epoch = instruction.key.epoch;
    receipt.gross = instruction.gross;
    receipt.fee = instruction.fee;
    receipt.net = instruction.net();
    receipt.obligations = instruction.obligations.size();
    return receipt;
}

void SettlementEngine::markOrdersSettled(const ReducedInstruction& instruction) {
    for (const auto& slice : instruction.obligations) {
        const auto& liability = ledger_.liability(slice.order);
        if (liability.complete()) {
            routes_.updateOrderStatus(slice.order, OrderStatus::Settled);
        }
    }
}

} // namespace tdtl
