#pragma once

#include "amount.hpp"
#include "event.hpp"
#include "identifiers.hpp"
#include "ledger.hpp"
#include "registry.hpp"
#include "route.hpp"

#include <map>
#include <string>
#include <vector>

namespace tdtl {

struct ObligationSlice {
    OrderId order;
    VaultId sourceVault;
    AccountId beneficiary;
    Amount amount{Amount::zero()};
    Amount fee{Amount::zero()};
};

struct ReducedKey {
    AssetId asset;
    DomainId destinationDomain;
    OperatorId operatorId;
    EpochId epoch;
    WindowId window;

    bool operator<(const ReducedKey& other) const;
    std::string str() const;
};

struct ReducedInstruction {
    ReducedKey key;
    VaultId releaseVault;
    Amount gross{Amount::zero()};
    Amount fee{Amount::zero()};
    std::vector<ObligationSlice> obligations;
    std::vector<std::string> legIds;

    Amount net() const;
    std::size_t orderCount() const;
};

struct ReductionStats {
    std::size_t inputLegs{0};
    std::size_t outputInstructions{0};
    Amount gross{Amount::zero()};
    Amount fee{Amount::zero()};
};

class PlanReducer {
public:
    PlanReducer(const Registry& registry, const Ledger& ledger, const RouteBook& routes);

    std::vector<ReducedInstruction> reduce(EpochId epoch, EventLog* events = nullptr) const;
    ReductionStats stats(const std::vector<ReducedInstruction>& instructions) const;

private:
    VaultId chooseReleaseVault(const VaultId& current, const VaultId& candidate) const;

    const Registry& registry_;
    const Ledger& ledger_;
    const RouteBook& routes_;
};

} // namespace tdtl
