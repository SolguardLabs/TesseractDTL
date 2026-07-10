#include "registry.hpp"

#include "errors.hpp"

#include <algorithm>

namespace tdtl {

bool LiquidityWindow::contains(EpochId epoch) const {
    return active && epoch >= openEpoch && epoch <= closeEpoch;
}

void Registry::addAsset(const Asset& asset, EventLog* events) {
    if (asset.id.empty()) {
        throw ValidationError("asset id is empty");
    }
    if (asset.decimals < 0 || asset.decimals > 18) {
        throw ValidationError("asset decimals out of range");
    }
    assets_[asset.id] = asset;
    if (events != nullptr) {
        Event event;
        event.type = EventType::AssetRegistered;
        event.subject = asset.id.str();
        event.fields["symbol"] = asset.symbol;
        event.fields["decimals"] = std::to_string(asset.decimals);
        events->push(event);
    }
}

void Registry::addDomain(const Domain& domain, EventLog* events) {
    if (domain.id.empty()) {
        throw ValidationError("domain id is empty");
    }
    if (domain.settlementDepth <= 0) {
        throw ValidationError("domain settlement depth must be positive");
    }
    domains_[domain.id] = domain;
    if (events != nullptr) {
        Event event;
        event.type = EventType::DomainRegistered;
        event.subject = domain.id.str();
        event.fields["name"] = domain.name;
        event.fields["depth"] = std::to_string(domain.settlementDepth);
        events->push(event);
    }
}

void Registry::addOperator(const Operator& op, EventLog* events) {
    if (op.id.empty()) {
        throw ValidationError("operator id is empty");
    }
    if (op.feeAccount.empty()) {
        throw ValidationError("operator fee account is empty");
    }
    for (const auto& domain : op.domains) {
        if (!hasDomain(domain)) {
            throw ValidationError("operator references missing domain " + domain.str());
        }
    }
    operators_[op.id] = op;
    if (events != nullptr) {
        Event event;
        event.type = EventType::OperatorRegistered;
        event.subject = op.id.str();
        event.fields["label"] = op.label;
        event.fields["feeAccount"] = op.feeAccount.str();
        events->push(event);
    }
}

void Registry::addWindow(const LiquidityWindow& window, EventLog* events) {
    if (window.id.empty()) {
        throw ValidationError("window id is empty");
    }
    if (window.closeEpoch < window.openEpoch) {
        throw ValidationError("window closes before it opens");
    }
    if (window.hardCap < window.softCap) {
        throw ValidationError("window hard cap below soft cap");
    }
    windows_[window.id] = window;
    if (events != nullptr) {
        Event event;
        event.type = EventType::WindowOpened;
        event.subject = window.id.str();
        event.epoch = window.openEpoch;
        event.fields["closeEpoch"] = window.closeEpoch.str();
        event.fields["hardCap"] = window.hardCap.str();
        events->push(event);
    }
}

void Registry::addVault(const VaultDescriptor& vault, EventLog* events) {
    if (vault.id.empty()) {
        throw ValidationError("vault id is empty");
    }
    if (!hasAsset(vault.asset)) {
        throw ValidationError("vault references missing asset " + vault.asset.str());
    }
    if (!hasDomain(vault.domain)) {
        throw ValidationError("vault references missing domain " + vault.domain.str());
    }
    if (vault.controller.empty()) {
        throw ValidationError("vault controller is empty");
    }
    vaults_[vault.id] = vault;
    if (events != nullptr) {
        Event event;
        event.type = EventType::VaultRegistered;
        event.subject = vault.id.str();
        event.fields["asset"] = vault.asset.str();
        event.fields["domain"] = vault.domain.str();
        event.fields["controller"] = vault.controller.str();
        events->push(event);
    }
}

const Asset& Registry::asset(const AssetId& id) const {
    auto it = assets_.find(id);
    if (it == assets_.end()) {
        throw ValidationError("unknown asset " + id.str());
    }
    return it->second;
}

const Domain& Registry::domain(const DomainId& id) const {
    auto it = domains_.find(id);
    if (it == domains_.end()) {
        throw ValidationError("unknown domain " + id.str());
    }
    return it->second;
}

const Operator& Registry::operatorInfo(const OperatorId& id) const {
    auto it = operators_.find(id);
    if (it == operators_.end()) {
        throw ValidationError("unknown operator " + id.str());
    }
    return it->second;
}

const LiquidityWindow& Registry::window(const WindowId& id) const {
    auto it = windows_.find(id);
    if (it == windows_.end()) {
        throw ValidationError("unknown window " + id.str());
    }
    return it->second;
}

const VaultDescriptor& Registry::vault(const VaultId& id) const {
    auto it = vaults_.find(id);
    if (it == vaults_.end()) {
        throw ValidationError("unknown vault " + id.str());
    }
    return it->second;
}

bool Registry::hasAsset(const AssetId& id) const {
    return assets_.find(id) != assets_.end();
}

bool Registry::hasDomain(const DomainId& id) const {
    return domains_.find(id) != domains_.end();
}

bool Registry::hasOperator(const OperatorId& id) const {
    return operators_.find(id) != operators_.end();
}

bool Registry::hasWindow(const WindowId& id) const {
    return windows_.find(id) != windows_.end();
}

bool Registry::hasVault(const VaultId& id) const {
    return vaults_.find(id) != vaults_.end();
}

bool Registry::operatorAllowed(
    const OperatorId& op,
    const DomainId& origin,
    const DomainId& dest) const {
    const auto& info = operatorInfo(op);
    if (!info.active) {
        return false;
    }
    return info.domains.find(origin) != info.domains.end() &&
           info.domains.find(dest) != info.domains.end();
}

bool Registry::epochOpen(const WindowId& windowId, EpochId epoch) const {
    return window(windowId).contains(epoch);
}

bool Registry::vaultMatches(
    const VaultId& vaultId,
    const AssetId& assetId,
    const DomainId& domainId) const {
    const auto& info = vault(vaultId);
    return info.asset == assetId && info.domain == domainId;
}

int Registry::vaultRank(const VaultId& vaultId) const {
    return vault(vaultId).liquidityRank;
}

std::vector<Asset> Registry::assets() const {
    std::vector<Asset> out;
    for (const auto& item : assets_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<Domain> Registry::domains() const {
    std::vector<Domain> out;
    for (const auto& item : domains_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<Operator> Registry::operators() const {
    std::vector<Operator> out;
    for (const auto& item : operators_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<LiquidityWindow> Registry::windows() const {
    std::vector<LiquidityWindow> out;
    for (const auto& item : windows_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<VaultDescriptor> Registry::vaults() const {
    std::vector<VaultDescriptor> out;
    for (const auto& item : vaults_) {
        out.push_back(item.second);
    }
    return out;
}

} // namespace tdtl
