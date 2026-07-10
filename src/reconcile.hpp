#pragma once

#include "amount.hpp"
#include "ledger.hpp"
#include "registry.hpp"
#include "settlement.hpp"

#include <map>
#include <string>
#include <vector>

namespace tdtl {

struct AssetReconciliation {
    AssetId asset;
    Amount vaultCash{Amount::zero()};
    Amount accounts{Amount::zero()};
    Amount pending{Amount::zero()};
    Amount released{Amount::zero()};
    Amount deposits{Amount::zero()};
    Amount swept{Amount::zero()};
    Amount supply{Amount::zero()};
    bool conservationOk{true};
};

struct DomainReconciliation {
    DomainId domain;
    AssetId asset;
    Amount vaultCash{Amount::zero()};
    Amount pending{Amount::zero()};
    Amount released{Amount::zero()};
    int vaults{0};
};

struct OperatorReconciliation {
    OperatorId operatorId;
    AssetId asset;
    Amount fees{Amount::zero()};
    Amount gross{Amount::zero()};
    int instructions{0};
};

struct ReconciliationReport {
    std::vector<AssetReconciliation> assets;
    std::vector<DomainReconciliation> domains;
    std::vector<OperatorReconciliation> operators;
    Amount openLiability{Amount::zero()};
    bool allVaultsHealthy{true};
    bool conservationOk{true};
};

class Reconciler {
public:
    Reconciler(const Registry& registry, const Ledger& ledger, const std::vector<EpochReceipt>& receipts);

    ReconciliationReport build() const;

private:
    std::vector<AssetReconciliation> assetReports() const;
    std::vector<DomainReconciliation> domainReports() const;
    std::vector<OperatorReconciliation> operatorReports() const;

    const Registry& registry_;
    const Ledger& ledger_;
    const std::vector<EpochReceipt>& receipts_;
};

} // namespace tdtl
