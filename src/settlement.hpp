#pragma once

#include "event.hpp"
#include "ledger.hpp"
#include "reducer.hpp"
#include "registry.hpp"
#include "route.hpp"

#include <set>
#include <vector>

namespace tdtl {

struct InstructionReceipt {
    std::string key;
    VaultId releaseVault;
    AssetId asset;
    OperatorId operatorId;
    EpochId epoch;
    Amount gross{Amount::zero()};
    Amount fee{Amount::zero()};
    Amount net{Amount::zero()};
    std::size_t obligations{0};
};

struct EpochReceipt {
    EpochId epoch;
    std::vector<InstructionReceipt> instructions;
    Amount gross{Amount::zero()};
    Amount fee{Amount::zero()};
    Amount net{Amount::zero()};
    bool settled{false};
};

class SettlementEngine {
public:
    SettlementEngine(Registry& registry, Ledger& ledger, RouteBook& routes, EventLog& events);

    EpochReceipt settleEpoch(EpochId epoch);
    std::vector<EpochReceipt> settleAll();
    std::vector<EpochReceipt> settleUntil(EpochId maxEpoch);
    bool isSettled(EpochId epoch) const;
    const std::vector<EpochReceipt>& receipts() const;

private:
    InstructionReceipt applyInstruction(const ReducedInstruction& instruction);
    void markOrdersSettled(const ReducedInstruction& instruction);

    Registry& registry_;
    Ledger& ledger_;
    RouteBook& routes_;
    EventLog& events_;
    std::set<int> settledEpochs_;
    std::vector<EpochReceipt> receipts_;
};

} // namespace tdtl
