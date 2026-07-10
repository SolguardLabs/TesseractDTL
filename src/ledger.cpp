#include "ledger.hpp"

#include "errors.hpp"

#include <algorithm>

namespace tdtl {

bool BalanceKey::operator<(const BalanceKey& other) const {
    if (account != other.account) {
        return account < other.account;
    }
    return asset < other.asset;
}

Amount VaultState::available() const {
    return cash - pendingLiability;
}

Amount VaultState::surplus() const {
    const Amount value = cash - pendingLiability;
    if (value.isNegative()) {
        return Amount::zero();
    }
    return value;
}

bool VaultState::healthy() const {
    return cash >= Amount::zero() && pendingLiability >= Amount::zero() && cash >= pendingLiability;
}

Amount LiabilityRecord::remaining() const {
    return amount - closed;
}

bool LiabilityRecord::complete() const {
    return closed >= amount;
}

void Ledger::registerVault(const VaultDescriptor& descriptor) {
    VaultState state;
    state.id = descriptor.id;
    state.asset = descriptor.asset;
    state.domain = descriptor.domain;
    state.controller = descriptor.controller;
    vaults_[descriptor.id] = state;
}

void Ledger::fundVault(const VaultId& vaultId, Amount amount) {
    if (amount.isNegative()) {
        throw LedgerError("cannot fund vault with negative amount");
    }
    auto& state = vaultMut(vaultId);
    state.cash += amount;
}

void Ledger::ensureAccount(const AccountId& account, const AssetId& asset) {
    BalanceKey key{account, asset};
    if (balances_.find(key) == balances_.end()) {
        balances_[key] = Amount::zero();
    }
}

void Ledger::credit(const AccountId& account, const AssetId& asset, Amount amount) {
    if (amount.isNegative()) {
        throw LedgerError("cannot credit negative amount");
    }
    ensureAccount(account, asset);
    balances_[BalanceKey{account, asset}] += amount;
}

void Ledger::debit(const AccountId& account, const AssetId& asset, Amount amount) {
    if (amount.isNegative()) {
        throw LedgerError("cannot debit negative amount");
    }
    ensureAccount(account, asset);
    auto& balance = balances_[BalanceKey{account, asset}];
    if (balance < amount) {
        throw LedgerError("insufficient account balance for " + account.str());
    }
    balance -= amount;
}

void Ledger::acceptDeposit(
    const OrderId& order,
    const AccountId& owner,
    const AccountId& beneficiary,
    const VaultDescriptor& descriptor,
    Amount amount,
    EpochId epoch,
    EventLog* events) {
    if (amount <= Amount::zero()) {
        throw LedgerError("deposit amount must be positive");
    }
    auto vit = vaults_.find(descriptor.id);
    if (vit == vaults_.end()) {
        throw LedgerError("vault not registered " + descriptor.id.str());
    }
    if (liabilities_.find(order) != liabilities_.end()) {
        throw LedgerError("duplicate order " + order.str());
    }
    debit(owner, descriptor.asset, amount);
    vit->second.cash += amount;
    vit->second.pendingLiability += amount;
    vit->second.deposits += amount;

    LiabilityRecord liability;
    liability.order = order;
    liability.vault = descriptor.id;
    liability.asset = descriptor.asset;
    liability.domain = descriptor.domain;
    liability.owner = owner;
    liability.beneficiary = beneficiary;
    liability.amount = amount;
    liability.epoch = epoch;
    liabilities_[order] = liability;

    if (events != nullptr) {
        Event event;
        event.type = EventType::DepositAccepted;
        event.epoch = epoch;
        event.subject = order.str();
        event.fields["owner"] = owner.str();
        event.fields["beneficiary"] = beneficiary.str();
        event.fields["vault"] = descriptor.id.str();
        event.fields["amount"] = amount.str();
        events->push(event);
    }
}

void Ledger::debitVault(const VaultId& vaultId, Amount amount) {
    if (amount.isNegative()) {
        throw LedgerError("cannot debit negative vault amount");
    }
    auto& state = vaultMut(vaultId);
    if (state.cash < amount) {
        throw LedgerError("insufficient vault cash " + vaultId.str());
    }
    state.cash -= amount;
}

void Ledger::releaseFromVault(const VaultId& vaultId, Amount amount, Amount fee) {
    if (amount.isNegative() || fee.isNegative()) {
        throw LedgerError("release amounts must be non-negative");
    }
    if (fee > amount) {
        throw LedgerError("fee exceeds release amount");
    }
    auto& state = vaultMut(vaultId);
    if (state.cash < amount) {
        throw LedgerError("insufficient release liquidity " + vaultId.str());
    }
    state.cash -= amount;
    state.released += amount;
    state.feesPaid += fee;
}

void Ledger::closeLiability(const OrderId& order, const VaultId& vaultId, Amount amount) {
    if (amount.isNegative()) {
        throw LedgerError("cannot close negative liability");
    }
    auto& record = liabilityMut(order);
    if (record.vault != vaultId) {
        throw LedgerError("liability vault mismatch for " + order.str());
    }
    if (record.remaining() < amount) {
        throw LedgerError("liability close exceeds remaining for " + order.str());
    }
    auto& state = vaultMut(vaultId);
    if (state.pendingLiability < amount) {
        throw LedgerError("vault pending liability underflow " + vaultId.str());
    }
    record.closed += amount;
    state.pendingLiability -= amount;
}

void Ledger::sweepSurplus(
    const VaultId& vaultId,
    const AccountId& controller,
    Amount amount,
    EventLog* events) {
    auto& state = vaultMut(vaultId);
    if (state.controller != controller) {
        throw LedgerError("controller mismatch for surplus sweep");
    }
    if (state.surplus() < amount) {
        throw LedgerError("surplus too low for sweep");
    }
    state.cash -= amount;
    state.swept += amount;
    credit(controller, state.asset, amount);
    if (events != nullptr) {
        Event event;
        event.type = EventType::SurplusSwept;
        event.subject = vaultId.str();
        event.fields["controller"] = controller.str();
        event.fields["amount"] = amount.str();
        events->push(event);
    }
}

const VaultState& Ledger::vault(const VaultId& id) const {
    auto it = vaults_.find(id);
    if (it == vaults_.end()) {
        throw LedgerError("unknown vault " + id.str());
    }
    return it->second;
}

VaultState& Ledger::vaultMut(const VaultId& id) {
    auto it = vaults_.find(id);
    if (it == vaults_.end()) {
        throw LedgerError("unknown vault " + id.str());
    }
    return it->second;
}

const LiabilityRecord& Ledger::liability(const OrderId& order) const {
    auto it = liabilities_.find(order);
    if (it == liabilities_.end()) {
        throw LedgerError("unknown liability " + order.str());
    }
    return it->second;
}

LiabilityRecord& Ledger::liabilityMut(const OrderId& order) {
    auto it = liabilities_.find(order);
    if (it == liabilities_.end()) {
        throw LedgerError("unknown liability " + order.str());
    }
    return it->second;
}

Amount Ledger::balanceOf(const AccountId& account, const AssetId& asset) const {
    auto it = balances_.find(BalanceKey{account, asset});
    if (it == balances_.end()) {
        return Amount::zero();
    }
    return it->second;
}

Amount Ledger::totalVaultCash(const AssetId& asset) const {
    Amount total = Amount::zero();
    for (const auto& item : vaults_) {
        if (item.second.asset == asset) {
            total += item.second.cash;
        }
    }
    return total;
}

Amount Ledger::totalPending(const AssetId& asset) const {
    Amount total = Amount::zero();
    for (const auto& item : vaults_) {
        if (item.second.asset == asset) {
            total += item.second.pendingLiability;
        }
    }
    return total;
}

Amount Ledger::totalReleased(const AssetId& asset) const {
    Amount total = Amount::zero();
    for (const auto& item : vaults_) {
        if (item.second.asset == asset) {
            total += item.second.released;
        }
    }
    return total;
}

Amount Ledger::totalAccounts(const AssetId& asset) const {
    Amount total = Amount::zero();
    for (const auto& item : balances_) {
        if (item.first.asset == asset) {
            total += item.second;
        }
    }
    return total;
}

Amount Ledger::totalDeposits(const AssetId& asset) const {
    Amount total = Amount::zero();
    for (const auto& item : vaults_) {
        if (item.second.asset == asset) {
            total += item.second.deposits;
        }
    }
    return total;
}

Amount Ledger::totalSwept(const AssetId& asset) const {
    Amount total = Amount::zero();
    for (const auto& item : vaults_) {
        if (item.second.asset == asset) {
            total += item.second.swept;
        }
    }
    return total;
}

std::vector<AccountBalance> Ledger::accounts() const {
    std::vector<AccountBalance> out;
    for (const auto& item : balances_) {
        AccountBalance balance;
        balance.account = item.first.account;
        balance.asset = item.first.asset;
        balance.amount = item.second;
        out.push_back(balance);
    }
    return out;
}

std::vector<VaultState> Ledger::vaults() const {
    std::vector<VaultState> out;
    for (const auto& item : vaults_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<LiabilityRecord> Ledger::liabilities() const {
    std::vector<LiabilityRecord> out;
    for (const auto& item : liabilities_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<LiabilityRecord> Ledger::liabilitiesForEpoch(EpochId epoch) const {
    std::vector<LiabilityRecord> out;
    for (const auto& item : liabilities_) {
        if (item.second.epoch == epoch) {
            out.push_back(item.second);
        }
    }
    return out;
}

} // namespace tdtl
