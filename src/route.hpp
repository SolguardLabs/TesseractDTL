#pragma once

#include "amount.hpp"
#include "event.hpp"
#include "identifiers.hpp"
#include "ledger.hpp"
#include "registry.hpp"

#include <map>
#include <string>
#include <vector>

namespace tdtl {

enum class OrderStatus {
    Draft,
    Deposited,
    Routed,
    Settled
};

std::string orderStatusName(OrderStatus status);

struct Order {
    OrderId id;
    AccountId owner;
    AccountId beneficiary;
    AssetId asset;
    DomainId originDomain;
    VaultId sourceVault;
    Amount amount{Amount::zero()};
    EpochId epoch;
    OrderStatus status{OrderStatus::Draft};

    bool payable() const;
};

struct RouteLeg {
    std::string legId;
    OrderId order;
    AssetId asset;
    DomainId originDomain;
    DomainId destinationDomain;
    VaultId sourceVault;
    VaultId releaseVault;
    OperatorId operatorId;
    WindowId window;
    Amount amount{Amount::zero()};
    Ratio feeRatio{Ratio::bps(0)};
    EpochId epoch;
    int hopIndex{0};

    Amount fee() const;
    Amount net() const;
    std::string dimensionalString() const;
};

struct RoutePlan {
    RouteId id;
    std::vector<RouteLeg> legs;
    std::string label;
    bool active{true};

    Amount totalAmount() const;
    Amount totalFee() const;
    EpochId minEpoch() const;
    EpochId maxEpoch() const;
    std::vector<RouteLeg> legsForEpoch(EpochId epoch) const;
};

class RouteBook {
public:
    void addOrder(const Order& order);
    void updateOrderStatus(const OrderId& order, OrderStatus status);
    void addRoute(const RoutePlan& route, EventLog* events = nullptr);

    const Order& order(const OrderId& order) const;
    Order& orderMut(const OrderId& order);
    const RoutePlan& route(const RouteId& route) const;
    RoutePlan& routeMut(const RouteId& route);

    bool hasOrder(const OrderId& order) const;
    bool hasRoute(const RouteId& route) const;

    std::vector<Order> orders() const;
    std::vector<RoutePlan> routes() const;
    std::vector<RouteLeg> legsForEpoch(EpochId epoch) const;
    std::vector<EpochId> epochs() const;

private:
    std::map<OrderId, Order> orders_;
    std::map<RouteId, RoutePlan> routes_;
};

class RouteValidator {
public:
    RouteValidator(const Registry& registry, const Ledger& ledger, const RouteBook& routes);

    void validateOrder(const Order& order) const;
    void validateLeg(const RouteLeg& leg) const;
    void validateRoute(const RoutePlan& route) const;
    void validateEpoch(EpochId epoch) const;

private:
    const Registry& registry_;
    const Ledger& ledger_;
    const RouteBook& routes_;
};

} // namespace tdtl
