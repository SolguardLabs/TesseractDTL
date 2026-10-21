#pragma once

#include "scenario.hpp"

#include <iosfwd>
#include <vector>

namespace tdtl {

struct VaultLiquidityView {
    VaultId vault;
    AssetId asset;
    DomainId domain;
    Amount cash{Amount::zero()};
    Amount pending{Amount::zero()};
    Amount released{Amount::zero()};
    Amount available{Amount::zero()};
    std::int64_t coverageBps{0};
    std::int64_t releaseShareBps{0};
};

class TreasuryAnalyzer {
public:
    static std::vector<VaultLiquidityView> vaults(const ScenarioState& state);
    static void writeJson(std::ostream& out, const ScenarioState& state);
};

} // namespace tdtl
