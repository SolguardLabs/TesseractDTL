#include "reconcile.hpp"

#include <map>
#include <tuple>

namespace tdtl {

Reconciler::Reconciler(
    const Registry& registry,
    const Ledger& ledger,
    const std::vector<EpochReceipt>& receipts)
    : registry_(registry), ledger_(ledger), receipts_(receipts) {}

ReconciliationReport Reconciler::build() const {
    ReconciliationReport report;
    report.assets = assetReports();
    report.domains = domainReports();
    report.operators = operatorReports();

    for (const auto& asset : report.assets) {
        report.openLiability += asset.pending;
        if (!asset.conservationOk) {
            report.conservationOk = false;
        }
    }
    for (const auto& vault : ledger_.vaults()) {
        if (!vault.healthy()) {
            report.allVaultsHealthy = false;
        }
    }
    return report;
}

std::vector<AssetReconciliation> Reconciler::assetReports() const {
    std::vector<AssetReconciliation> out;
    for (const auto& asset : registry_.assets()) {
        AssetReconciliation row;
        row.asset = asset.id;
        row.vaultCash = ledger_.totalVaultCash(asset.id);
        row.accounts = ledger_.totalAccounts(asset.id);
        row.pending = ledger_.totalPending(asset.id);
        row.released = ledger_.totalReleased(asset.id);
        row.deposits = ledger_.totalDeposits(asset.id);
        row.swept = ledger_.totalSwept(asset.id);
        row.supply = row.vaultCash + row.accounts;
        row.conservationOk = row.supply >= Amount::zero();
        out.push_back(row);
    }
    return out;
}

std::vector<DomainReconciliation> Reconciler::domainReports() const {
    std::map<std::tuple<DomainId, AssetId>, DomainReconciliation> map;
    for (const auto& vault : ledger_.vaults()) {
        auto key = std::make_tuple(vault.domain, vault.asset);
        auto& row = map[key];
        row.domain = vault.domain;
        row.asset = vault.asset;
        row.vaultCash += vault.cash;
        row.pending += vault.pendingLiability;
        row.released += vault.released;
        row.vaults += 1;
    }
    std::vector<DomainReconciliation> out;
    for (const auto& item : map) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<OperatorReconciliation> Reconciler::operatorReports() const {
    std::map<std::tuple<OperatorId, AssetId>, OperatorReconciliation> map;
    for (const auto& epoch : receipts_) {
        for (const auto& instruction : epoch.instructions) {
            auto key = std::make_tuple(instruction.operatorId, instruction.asset);
            auto& row = map[key];
            row.operatorId = instruction.operatorId;
            row.asset = instruction.asset;
            row.fees += instruction.fee;
            row.gross += instruction.gross;
            row.instructions += 1;
        }
    }
    std::vector<OperatorReconciliation> out;
    for (const auto& item : map) {
        out.push_back(item.second);
    }
    return out;
}

} // namespace tdtl
