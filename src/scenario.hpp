#pragma once

#include "event.hpp"
#include "ledger.hpp"
#include "reconcile.hpp"
#include "registry.hpp"
#include "route.hpp"
#include "settlement.hpp"

#include <optional>
#include <string>
#include <vector>

namespace tdtl {

struct ScenarioInfo {
    std::string name;
    std::string title;
    std::string description;
    int expectedOrders{0};
    int expectedRoutes{0};
};

struct ScenarioState {
    std::string name;
    Registry registry;
    Ledger ledger;
    RouteBook routes;
    EventLog events;
    std::vector<EpochReceipt> receipts;
    ReconciliationReport reconciliation;
};

class ScenarioBuilder {
public:
    std::vector<ScenarioInfo> list() const;
    bool exists(const std::string& name) const;
    ScenarioState build(const std::string& name, std::optional<EpochId> maxEpoch = std::nullopt) const;

private:
    ScenarioState base(const std::string& name) const;
    void seedRegistry(ScenarioState& state) const;
    void seedAccounts(ScenarioState& state) const;
    void seedVaultLiquidity(ScenarioState& state) const;

    void submitOrder(
        ScenarioState& state,
        const OrderId& orderId,
        const AccountId& owner,
        const AccountId& beneficiary,
        const AssetId& asset,
        const DomainId& originDomain,
        const VaultId& sourceVault,
        Amount amount,
        EpochId epoch) const;

    RouteLeg makeLeg(
        const std::string& legId,
        const OrderId& orderId,
        const AssetId& asset,
        const DomainId& origin,
        const DomainId& destination,
        const VaultId& sourceVault,
        const VaultId& releaseVault,
        const OperatorId& operatorId,
        const WindowId& window,
        Amount amount,
        std::int64_t feeBps,
        EpochId epoch,
        int hopIndex) const;

    void buildBalanced(ScenarioState& state) const;
    void buildMultiEpoch(ScenarioState& state) const;
    void buildOperatorWindows(ScenarioState& state) const;
    void buildMeshReconcile(ScenarioState& state) const;
    void validateRoutes(const ScenarioState& state) const;
    void settle(ScenarioState& state, std::optional<EpochId> maxEpoch) const;
};

} // namespace tdtl
