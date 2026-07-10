#pragma once

#include "amount.hpp"
#include "event.hpp"
#include "identifiers.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace tdtl {

struct Asset {
    AssetId id;
    std::string symbol;
    int decimals{6};
    Amount minOrder{Amount::zero()};
    Amount maxOrder{Amount::zero()};
};

struct Domain {
    DomainId id;
    std::string name;
    int settlementDepth{1};
    bool active{true};
};

struct Operator {
    OperatorId id;
    AccountId feeAccount;
    std::string label;
    std::set<DomainId> domains;
    Amount dailyLimit{Amount::zero()};
    bool active{true};
};

struct LiquidityWindow {
    WindowId id;
    EpochId openEpoch;
    EpochId closeEpoch;
    Amount softCap{Amount::zero()};
    Amount hardCap{Amount::zero()};
    bool active{true};

    bool contains(EpochId epoch) const;
};

struct VaultDescriptor {
    VaultId id;
    AssetId asset;
    DomainId domain;
    AccountId controller;
    std::string label;
    int liquidityRank{0};
    bool acceptsDeposits{true};
    bool releasesLiquidity{true};
};

class Registry {
public:
    void addAsset(const Asset& asset, EventLog* events = nullptr);
    void addDomain(const Domain& domain, EventLog* events = nullptr);
    void addOperator(const Operator& op, EventLog* events = nullptr);
    void addWindow(const LiquidityWindow& window, EventLog* events = nullptr);
    void addVault(const VaultDescriptor& vault, EventLog* events = nullptr);

    const Asset& asset(const AssetId& id) const;
    const Domain& domain(const DomainId& id) const;
    const Operator& operatorInfo(const OperatorId& id) const;
    const LiquidityWindow& window(const WindowId& id) const;
    const VaultDescriptor& vault(const VaultId& id) const;

    bool hasAsset(const AssetId& id) const;
    bool hasDomain(const DomainId& id) const;
    bool hasOperator(const OperatorId& id) const;
    bool hasWindow(const WindowId& id) const;
    bool hasVault(const VaultId& id) const;

    bool operatorAllowed(const OperatorId& op, const DomainId& origin, const DomainId& dest) const;
    bool epochOpen(const WindowId& window, EpochId epoch) const;
    bool vaultMatches(const VaultId& vault, const AssetId& asset, const DomainId& domain) const;
    int vaultRank(const VaultId& vault) const;

    std::vector<Asset> assets() const;
    std::vector<Domain> domains() const;
    std::vector<Operator> operators() const;
    std::vector<LiquidityWindow> windows() const;
    std::vector<VaultDescriptor> vaults() const;

private:
    std::map<AssetId, Asset> assets_;
    std::map<DomainId, Domain> domains_;
    std::map<OperatorId, Operator> operators_;
    std::map<WindowId, LiquidityWindow> windows_;
    std::map<VaultId, VaultDescriptor> vaults_;
};

} // namespace tdtl
