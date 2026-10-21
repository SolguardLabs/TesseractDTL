#include "treasury.hpp"

#include "report.hpp"

#include <cstdint>
#include <limits>
#include <map>

namespace tdtl {

namespace {

std::int64_t basisPoints(Amount numerator, Amount denominator) {
    if (denominator <= Amount::zero()) {
        return 0;
    }
    const auto whole = numerator.raw() / denominator.raw();
    const auto remainder = numerator.raw() % denominator.raw();
    if (whole > std::numeric_limits<std::int64_t>::max() / 10'000) {
        return std::numeric_limits<std::int64_t>::max();
    }
    return whole * 10'000 + (remainder * 10'000) / denominator.raw();
}

} // namespace

std::vector<VaultLiquidityView> TreasuryAnalyzer::vaults(const ScenarioState& state) {
    std::map<AssetId, Amount> assetReleased;
    const auto source = state.ledger.vaults();
    for (const auto& vault : source) {
        assetReleased[vault.asset] += vault.released;
    }

    std::vector<VaultLiquidityView> views;
    views.reserve(source.size());
    for (const auto& vault : source) {
        VaultLiquidityView view;
        view.vault = vault.id;
        view.asset = vault.asset;
        view.domain = vault.domain;
        view.cash = vault.cash;
        view.pending = vault.pendingLiability;
        view.released = vault.released;
        view.available = vault.available();
        view.coverageBps = basisPoints(vault.cash, vault.pendingLiability);
        view.releaseShareBps = basisPoints(vault.released, assetReleased.at(vault.asset));
        views.push_back(view);
    }
    return views;
}

void TreasuryAnalyzer::writeJson(std::ostream& out, const ScenarioState& state) {
    JsonStream json(out);
    json.objectBegin();
    json.key("scenario");
    json.string(state.name);
    json.comma();
    json.key("vaults");
    json.arrayBegin();
    const auto rows = vaults(state);
    for (std::size_t i = 0; i < rows.size(); ++i) {
        if (i > 0) {
            json.comma();
        }
        const auto& row = rows[i];
        json.objectBegin();
        json.key("vault"); json.string(row.vault.str()); json.comma();
        json.key("asset"); json.string(row.asset.str()); json.comma();
        json.key("domain"); json.string(row.domain.str()); json.comma();
        json.key("cash"); json.number(row.cash.raw()); json.comma();
        json.key("pending"); json.number(row.pending.raw()); json.comma();
        json.key("released"); json.number(row.released.raw()); json.comma();
        json.key("available"); json.number(row.available.raw()); json.comma();
        json.key("coverageBps"); json.number(row.coverageBps); json.comma();
        json.key("releaseShareBps"); json.number(row.releaseShareBps);
        json.objectEnd();
    }
    json.arrayEnd();
    json.objectEnd();
    out << '\n';
}

} // namespace tdtl
