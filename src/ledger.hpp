#pragma once

#include "amount.hpp"
#include "event.hpp"
#include "identifiers.hpp"
#include "registry.hpp"

#include <map>
#include <string>
#include <vector>

namespace tdtl {

struct BalanceKey {
    AccountId account;
    AssetId asset;

    bool operator<(const BalanceKey& other) const;
};

struct AccountBalance {
    AccountId account;
    AssetId asset;
    Amount amount{Amount::zero()};
};

struct VaultState {
    VaultId id;
    AssetId asset;
    DomainId domain;
    AccountId controller;
    Amount cash{Amount::zero()};
    Amount pendingLiability{Amount::zero()};
    Amount released{Amount::zero()};
    Amount feesPaid{Amount::zero()};
    Amount deposits{Amount::zero()};
    Amount swept{Amount::zero()};

    Amount available() const;
    Amount surplus() const;
    bool healthy() const;
};

struct LiabilityRecord {
    OrderId order;
    VaultId vault;
    AssetId asset;
    DomainId domain;
    AccountId owner;
    AccountId beneficiary;
    Amount amount{Amount::zero()};
    Amount closed{Amount::zero()};
    EpochId epoch;

    Amount remaining() const;
    bool complete() const;
};

class Ledger {
public:
    void registerVault(const VaultDescriptor& descriptor);
    void fundVault(const VaultId& vault, Amount amount);
    void ensureAccount(const AccountId& account, const AssetId& asset);
    void credit(const AccountId& account, const AssetId& asset, Amount amount);
    void debit(const AccountId& account, const AssetId& asset, Amount amount);

    void acceptDeposit(
        const OrderId& order,
        const AccountId& owner,
        const AccountId& beneficiary,
        const VaultDescriptor& descriptor,
        Amount amount,
        EpochId epoch,
        EventLog* events = nullptr);

    void debitVault(const VaultId& vault, Amount amount);
    void releaseFromVault(const VaultId& vault, Amount amount, Amount fee);
    void closeLiability(const OrderId& order, const VaultId& vault, Amount amount);
    void sweepSurplus(
        const VaultId& vault,
        const AccountId& controller,
        Amount amount,
        EventLog* events = nullptr);

    const VaultState& vault(const VaultId& id) const;
    VaultState& vaultMut(const VaultId& id);
    const LiabilityRecord& liability(const OrderId& order) const;
    LiabilityRecord& liabilityMut(const OrderId& order);

    Amount balanceOf(const AccountId& account, const AssetId& asset) const;
    Amount totalVaultCash(const AssetId& asset) const;
    Amount totalPending(const AssetId& asset) const;
    Amount totalReleased(const AssetId& asset) const;
    Amount totalAccounts(const AssetId& asset) const;
    Amount totalDeposits(const AssetId& asset) const;
    Amount totalSwept(const AssetId& asset) const;

    std::vector<AccountBalance> accounts() const;
    std::vector<VaultState> vaults() const;
    std::vector<LiabilityRecord> liabilities() const;
    std::vector<LiabilityRecord> liabilitiesForEpoch(EpochId epoch) const;

private:
    std::map<VaultId, VaultState> vaults_;
    std::map<OrderId, LiabilityRecord> liabilities_;
    std::map<BalanceKey, Amount> balances_;
};

} // namespace tdtl
