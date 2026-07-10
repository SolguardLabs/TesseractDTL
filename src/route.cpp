#include "route.hpp"

#include "errors.hpp"

#include <algorithm>
#include <set>
#include <sstream>

namespace tdtl {

std::string orderStatusName(OrderStatus status) {
    switch (status) {
    case OrderStatus::Draft:
        return "draft";
    case OrderStatus::Deposited:
        return "deposited";
    case OrderStatus::Routed:
        return "routed";
    case OrderStatus::Settled:
        return "settled";
    }
    return "unknown";
}

bool Order::payable() const {
    return status == OrderStatus::Deposited || status == OrderStatus::Routed;
}

Amount RouteLeg::fee() const {
    return feeRatio.apply(amount);
}

Amount RouteLeg::net() const {
    return amount - fee();
}

std::string RouteLeg::dimensionalString() const {
    std::ostringstream os;
    os << asset.str();
    os << "/";
    os << originDomain.str();
    os << "->";
    os << destinationDomain.str();
    os << "/";
    os << operatorId.str();
    os << "/";
    os << window.str();
    os << "/";
    os << epoch.value;
    return os.str();
}

Amount RoutePlan::totalAmount() const {
    Amount total = Amount::zero();
    for (const auto& leg : legs) {
        total += leg.amount;
    }
    return total;
}

Amount RoutePlan::totalFee() const {
    Amount total = Amount::zero();
    for (const auto& leg : legs) {
        total += leg.fee();
    }
    return total;
}

EpochId RoutePlan::minEpoch() const {
    if (legs.empty()) {
        return EpochId(0);
    }
    EpochId best = legs.front().epoch;
    for (const auto& leg : legs) {
        if (leg.epoch < best) {
            best = leg.epoch;
        }
    }
    return best;
}

EpochId RoutePlan::maxEpoch() const {
    if (legs.empty()) {
        return EpochId(0);
    }
    EpochId best = legs.front().epoch;
    for (const auto& leg : legs) {
        if (leg.epoch > best) {
            best = leg.epoch;
        }
    }
    return best;
}

std::vector<RouteLeg> RoutePlan::legsForEpoch(EpochId epoch) const {
    std::vector<RouteLeg> out;
    for (const auto& leg : legs) {
        if (leg.epoch == epoch) {
            out.push_back(leg);
        }
    }
    return out;
}

void RouteBook::addOrder(const Order& order) {
    if (order.id.empty()) {
        throw ValidationError("order id is empty");
    }
    if (orders_.find(order.id) != orders_.end()) {
        throw ValidationError("duplicate order " + order.id.str());
    }
    orders_[order.id] = order;
}

void RouteBook::updateOrderStatus(const OrderId& orderId, OrderStatus status) {
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        throw ValidationError("unknown order " + orderId.str());
    }
    it->second.status = status;
}

void RouteBook::addRoute(const RoutePlan& route, EventLog* events) {
    if (route.id.empty()) {
        throw ValidationError("route id is empty");
    }
    if (route.legs.empty()) {
        throw ValidationError("route has no legs");
    }
    if (routes_.find(route.id) != routes_.end()) {
        throw ValidationError("duplicate route " + route.id.str());
    }
    routes_[route.id] = route;
    for (const auto& leg : route.legs) {
        auto it = orders_.find(leg.order);
        if (it != orders_.end()) {
            it->second.status = OrderStatus::Routed;
        }
    }
    if (events != nullptr) {
        Event event;
        event.type = EventType::RouteCreated;
        event.epoch = route.minEpoch();
        event.subject = route.id.str();
        event.fields["label"] = route.label;
        event.fields["legs"] = std::to_string(route.legs.size());
        event.fields["amount"] = route.totalAmount().str();
        events->push(event);
    }
}

const Order& RouteBook::order(const OrderId& orderId) const {
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        throw ValidationError("unknown order " + orderId.str());
    }
    return it->second;
}

Order& RouteBook::orderMut(const OrderId& orderId) {
    auto it = orders_.find(orderId);
    if (it == orders_.end()) {
        throw ValidationError("unknown order " + orderId.str());
    }
    return it->second;
}

const RoutePlan& RouteBook::route(const RouteId& routeId) const {
    auto it = routes_.find(routeId);
    if (it == routes_.end()) {
        throw ValidationError("unknown route " + routeId.str());
    }
    return it->second;
}

RoutePlan& RouteBook::routeMut(const RouteId& routeId) {
    auto it = routes_.find(routeId);
    if (it == routes_.end()) {
        throw ValidationError("unknown route " + routeId.str());
    }
    return it->second;
}

bool RouteBook::hasOrder(const OrderId& orderId) const {
    return orders_.find(orderId) != orders_.end();
}

bool RouteBook::hasRoute(const RouteId& routeId) const {
    return routes_.find(routeId) != routes_.end();
}

std::vector<Order> RouteBook::orders() const {
    std::vector<Order> out;
    for (const auto& item : orders_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<RoutePlan> RouteBook::routes() const {
    std::vector<RoutePlan> out;
    for (const auto& item : routes_) {
        out.push_back(item.second);
    }
    return out;
}

std::vector<RouteLeg> RouteBook::legsForEpoch(EpochId epoch) const {
    std::vector<RouteLeg> out;
    for (const auto& item : routes_) {
        const auto legs = item.second.legsForEpoch(epoch);
        out.insert(out.end(), legs.begin(), legs.end());
    }
    std::sort(out.begin(), out.end(), [](const RouteLeg& left, const RouteLeg& right) {
        if (left.epoch != right.epoch) {
            return left.epoch < right.epoch;
        }
        if (left.hopIndex != right.hopIndex) {
            return left.hopIndex < right.hopIndex;
        }
        return left.legId < right.legId;
    });
    return out;
}

std::vector<EpochId> RouteBook::epochs() const {
    std::set<int> values;
    for (const auto& routeItem : routes_) {
        for (const auto& leg : routeItem.second.legs) {
            values.insert(leg.epoch.value);
        }
    }
    std::vector<EpochId> out;
    for (int value : values) {
        out.push_back(EpochId(value));
    }
    return out;
}

RouteValidator::RouteValidator(
    const Registry& registry,
    const Ledger& ledger,
    const RouteBook& routes)
    : registry_(registry), ledger_(ledger), routes_(routes) {}

void RouteValidator::validateOrder(const Order& order) const {
    if (!registry_.hasAsset(order.asset)) {
        throw ValidationError("order references missing asset " + order.asset.str());
    }
    if (!registry_.hasDomain(order.originDomain)) {
        throw ValidationError("order references missing domain " + order.originDomain.str());
    }
    if (!registry_.hasVault(order.sourceVault)) {
        throw ValidationError("order references missing vault " + order.sourceVault.str());
    }
    if (!registry_.vaultMatches(order.sourceVault, order.asset, order.originDomain)) {
        throw ValidationError("order source vault does not match asset/domain");
    }
    if (order.amount <= Amount::zero()) {
        throw ValidationError("order amount must be positive");
    }
    const auto& asset = registry_.asset(order.asset);
    if (asset.minOrder > Amount::zero() && order.amount < asset.minOrder) {
        throw ValidationError("order amount below asset minimum");
    }
    if (asset.maxOrder > Amount::zero() && order.amount > asset.maxOrder) {
        throw ValidationError("order amount above asset maximum");
    }
}

void RouteValidator::validateLeg(const RouteLeg& leg) const {
    if (!routes_.hasOrder(leg.order)) {
        throw ValidationError("leg references missing order " + leg.order.str());
    }
    const auto& order = routes_.order(leg.order);
    if (!order.payable()) {
        throw ValidationError("order is not payable " + leg.order.str());
    }
    if (leg.asset != order.asset) {
        throw ValidationError("leg asset does not match order");
    }
    if (leg.sourceVault != order.sourceVault) {
        throw ValidationError("leg source vault does not match order");
    }
    if (!registry_.hasDomain(leg.destinationDomain)) {
        throw ValidationError("leg references missing destination domain");
    }
    if (!registry_.vaultMatches(leg.sourceVault, leg.asset, leg.originDomain)) {
        throw ValidationError("leg source vault dimension mismatch");
    }
    if (!registry_.vaultMatches(leg.releaseVault, leg.asset, leg.destinationDomain)) {
        throw ValidationError("leg release vault dimension mismatch");
    }
    if (!registry_.operatorAllowed(leg.operatorId, leg.originDomain, leg.destinationDomain)) {
        throw ValidationError("operator is not authorized for route domains");
    }
    if (!registry_.epochOpen(leg.window, leg.epoch)) {
        throw ValidationError("leg epoch is outside liquidity window");
    }
    if (leg.amount <= Amount::zero()) {
        throw ValidationError("leg amount must be positive");
    }
    if (leg.amount > ledger_.liability(leg.order).remaining()) {
        throw ValidationError("leg amount exceeds remaining order liability");
    }
    if (leg.fee() < Amount::zero()) {
        throw ValidationError("leg fee is negative");
    }
    if (leg.fee() > leg.amount) {
        throw ValidationError("leg fee exceeds amount");
    }
}

void RouteValidator::validateRoute(const RoutePlan& route) const {
    if (!route.active) {
        throw ValidationError("route is not active");
    }
    if (route.legs.empty()) {
        throw ValidationError("route has no legs");
    }
    std::map<OrderId, Amount> routed;
    for (const auto& leg : route.legs) {
        validateLeg(leg);
        routed[leg.order] += leg.amount;
    }
    for (const auto& item : routed) {
        const auto& liability = ledger_.liability(item.first);
        if (item.second > liability.remaining()) {
            throw ValidationError("route overallocates order " + item.first.str());
        }
    }
}

void RouteValidator::validateEpoch(EpochId epoch) const {
    const auto legs = routes_.legsForEpoch(epoch);
    for (const auto& leg : legs) {
        validateLeg(leg);
    }
}

} // namespace tdtl
