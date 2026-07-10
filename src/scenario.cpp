#include "scenario.hpp"

#include "errors.hpp"

#include <algorithm>
#include <set>

namespace tdtl {

std::vector<ScenarioInfo> ScenarioBuilder::list() const {
    return {
        ScenarioInfo{
            "balanced",
            "Balanced two-domain settlement",
            "Deposits two ZUSD orders, creates a single route and settles the first epoch.",
            2,
            1,
        },
        ScenarioInfo{
            "multi-epoch",
            "Multi-epoch rolling settlement",
            "Splits settlement across two liquidity windows with independent reconciliation.",
            4,
            2,
        },
        ScenarioInfo{
            "operator-windows",
            "Operator and window policy controls",
            "Exercises two operators with distinct windows and domains.",
            3,
            2,
        },
        ScenarioInfo{
            "mesh-reconcile",
            "Tri-domain mesh reconciliation",
            "Builds a denser route graph across three domains and verifies final accounting.",
            5,
            3,
        },
    };
}

bool ScenarioBuilder::exists(const std::string& name) const {
    for (const auto& item : list()) {
        if (item.name == name) {
            return true;
        }
    }
    return false;
}

ScenarioState ScenarioBuilder::build(
    const std::string& name,
    std::optional<EpochId> maxEpoch) const {
    if (!exists(name)) {
        throw ScenarioError("unknown scenario " + name);
    }
    ScenarioState state = base(name);
    if (name == "balanced") {
        buildBalanced(state);
    } else if (name == "multi-epoch") {
        buildMultiEpoch(state);
    } else if (name == "operator-windows") {
        buildOperatorWindows(state);
    } else if (name == "mesh-reconcile") {
        buildMeshReconcile(state);
    }
    validateRoutes(state);
    settle(state, maxEpoch);
    return state;
}

ScenarioState ScenarioBuilder::base(const std::string& name) const {
    ScenarioState state;
    state.name = name;
    seedRegistry(state);
    seedAccounts(state);
    seedVaultLiquidity(state);
    return state;
}

void ScenarioBuilder::seedRegistry(ScenarioState& state) const {
    state.registry.addAsset(
        Asset{
            AssetId("ZUSD"),
            "ZUSD",
            6,
            Amount::of(10'000),
            Amount::of(20'000'000'000),
        },
        &state.events);
    state.registry.addAsset(
        Asset{
            AssetId("ZBTC"),
            "ZBTC",
            8,
            Amount::of(100),
            Amount::of(500'000'000),
        },
        &state.events);

    state.registry.addDomain(Domain{DomainId("alpha"), "Alpha rollup", 1, true}, &state.events);
    state.registry.addDomain(Domain{DomainId("beta"), "Beta execution domain", 2, true}, &state.events);
    state.registry.addDomain(Domain{DomainId("gamma"), "Gamma liquidity zone", 3, true}, &state.events);
    state.registry.addDomain(Domain{DomainId("delta"), "Delta archive lane", 4, true}, &state.events);

    Operator atlas;
    atlas.id = OperatorId("op-atlas");
    atlas.feeAccount = AccountId("fees-atlas");
    atlas.label = "Atlas routing desk";
    atlas.domains = {DomainId("alpha"), DomainId("beta"), DomainId("gamma")};
    atlas.dailyLimit = Amount::of(40'000'000'000);
    state.registry.addOperator(atlas, &state.events);

    Operator nova;
    nova.id = OperatorId("op-nova");
    nova.feeAccount = AccountId("fees-nova");
    nova.label = "Nova settlement desk";
    nova.domains = {DomainId("beta"), DomainId("gamma"), DomainId("delta")};
    nova.dailyLimit = Amount::of(35'000'000'000);
    state.registry.addOperator(nova, &state.events);

    Operator prism;
    prism.id = OperatorId("op-prism");
    prism.feeAccount = AccountId("fees-prism");
    prism.label = "Prism liquidity coordinator";
    prism.domains = {DomainId("alpha"), DomainId("beta"), DomainId("gamma"), DomainId("delta")};
    prism.dailyLimit = Amount::of(60'000'000'000);
    state.registry.addOperator(prism, &state.events);

    state.registry.addWindow(
        LiquidityWindow{
            WindowId("w-fast"),
            EpochId(1),
            EpochId(3),
            Amount::of(15'000'000'000),
            Amount::of(25'000'000'000),
            true,
        },
        &state.events);
    state.registry.addWindow(
        LiquidityWindow{
            WindowId("w-roll"),
            EpochId(2),
            EpochId(5),
            Amount::of(12'000'000'000),
            Amount::of(30'000'000'000),
            true,
        },
        &state.events);
    state.registry.addWindow(
        LiquidityWindow{
            WindowId("w-deep"),
            EpochId(1),
            EpochId(6),
            Amount::of(18'000'000'000),
            Amount::of(45'000'000'000),
            true,
        },
        &state.events);

    const std::vector<VaultDescriptor> vaults = {
        VaultDescriptor{
            VaultId("vault-alpha-zusd-a"),
            AssetId("ZUSD"),
            DomainId("alpha"),
            AccountId("controller-alpha"),
            "Alpha ZUSD primary",
            10,
            true,
            true,
        },
        VaultDescriptor{
            VaultId("vault-alpha-zusd-b"),
            AssetId("ZUSD"),
            DomainId("alpha"),
            AccountId("controller-alpha"),
            "Alpha ZUSD reserve",
            12,
            true,
            true,
        },
        VaultDescriptor{
            VaultId("vault-beta-zusd-a"),
            AssetId("ZUSD"),
            DomainId("beta"),
            AccountId("controller-beta"),
            "Beta ZUSD primary",
            20,
            true,
            true,
        },
        VaultDescriptor{
            VaultId("vault-beta-zusd-b"),
            AssetId("ZUSD"),
            DomainId("beta"),
            AccountId("controller-beta"),
            "Beta ZUSD deep",
            30,
            true,
            true,
        },
        VaultDescriptor{
            VaultId("vault-gamma-zusd-a"),
            AssetId("ZUSD"),
            DomainId("gamma"),
            AccountId("controller-gamma"),
            "Gamma ZUSD primary",
            25,
            true,
            true,
        },
        VaultDescriptor{
            VaultId("vault-delta-zusd-a"),
            AssetId("ZUSD"),
            DomainId("delta"),
            AccountId("controller-delta"),
            "Delta ZUSD archive",
            15,
            true,
            true,
        },
        VaultDescriptor{
            VaultId("vault-alpha-zbtc-a"),
            AssetId("ZBTC"),
            DomainId("alpha"),
            AccountId("controller-alpha"),
            "Alpha ZBTC primary",
            10,
            true,
            true,
        },
        VaultDescriptor{
            VaultId("vault-beta-zbtc-a"),
            AssetId("ZBTC"),
            DomainId("beta"),
            AccountId("controller-beta"),
            "Beta ZBTC primary",
            20,
            true,
            true,
        },
    };

    for (const auto& vault : vaults) {
        state.registry.addVault(vault, &state.events);
        state.ledger.registerVault(vault);
    }
}

void ScenarioBuilder::seedAccounts(ScenarioState& state) const {
    const std::vector<AccountId> accounts = {
        AccountId("alice"),
        AccountId("bob"),
        AccountId("carol"),
        AccountId("dan"),
        AccountId("erin"),
        AccountId("frank"),
        AccountId("beneficiary-alpha"),
        AccountId("beneficiary-beta"),
        AccountId("beneficiary-gamma"),
        AccountId("beneficiary-delta"),
        AccountId("fees-atlas"),
        AccountId("fees-nova"),
        AccountId("fees-prism"),
        AccountId("controller-alpha"),
        AccountId("controller-beta"),
        AccountId("controller-gamma"),
        AccountId("controller-delta"),
    };

    for (const auto& account : accounts) {
        state.ledger.ensureAccount(account, AssetId("ZUSD"));
        state.ledger.ensureAccount(account, AssetId("ZBTC"));
    }

    state.ledger.credit(AccountId("alice"), AssetId("ZUSD"), Amount::of(15'000'000'000));
    state.ledger.credit(AccountId("bob"), AssetId("ZUSD"), Amount::of(12'000'000'000));
    state.ledger.credit(AccountId("carol"), AssetId("ZUSD"), Amount::of(10'000'000'000));
    state.ledger.credit(AccountId("dan"), AssetId("ZUSD"), Amount::of(8'000'000'000));
    state.ledger.credit(AccountId("erin"), AssetId("ZUSD"), Amount::of(9'000'000'000));
    state.ledger.credit(AccountId("frank"), AssetId("ZUSD"), Amount::of(6'000'000'000));
    state.ledger.credit(AccountId("alice"), AssetId("ZBTC"), Amount::of(2'000'000));
    state.ledger.credit(AccountId("bob"), AssetId("ZBTC"), Amount::of(3'000'000));
}

void ScenarioBuilder::seedVaultLiquidity(ScenarioState& state) const {
    state.ledger.fundVault(VaultId("vault-alpha-zusd-a"), Amount::of(3'000'000'000));
    state.ledger.fundVault(VaultId("vault-alpha-zusd-b"), Amount::of(2'000'000'000));
    state.ledger.fundVault(VaultId("vault-beta-zusd-a"), Amount::of(9'000'000'000));
    state.ledger.fundVault(VaultId("vault-beta-zusd-b"), Amount::of(14'000'000'000));
    state.ledger.fundVault(VaultId("vault-gamma-zusd-a"), Amount::of(11'000'000'000));
    state.ledger.fundVault(VaultId("vault-delta-zusd-a"), Amount::of(7'000'000'000));
    state.ledger.fundVault(VaultId("vault-alpha-zbtc-a"), Amount::of(1'000'000));
    state.ledger.fundVault(VaultId("vault-beta-zbtc-a"), Amount::of(2'500'000));
}

void ScenarioBuilder::submitOrder(
    ScenarioState& state,
    const OrderId& orderId,
    const AccountId& owner,
    const AccountId& beneficiary,
    const AssetId& asset,
    const DomainId& originDomain,
    const VaultId& sourceVault,
    Amount amount,
    EpochId epoch) const {
    Order order;
    order.id = orderId;
    order.owner = owner;
    order.beneficiary = beneficiary;
    order.asset = asset;
    order.originDomain = originDomain;
    order.sourceVault = sourceVault;
    order.amount = amount;
    order.epoch = epoch;

    RouteValidator validator(state.registry, state.ledger, state.routes);
    validator.validateOrder(order);
    state.routes.addOrder(order);
    state.ledger.acceptDeposit(
        orderId,
        owner,
        beneficiary,
        state.registry.vault(sourceVault),
        amount,
        epoch,
        &state.events);
    state.routes.updateOrderStatus(orderId, OrderStatus::Deposited);
}

RouteLeg ScenarioBuilder::makeLeg(
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
    int hopIndex) const {
    RouteLeg leg;
    leg.legId = legId;
    leg.order = orderId;
    leg.asset = asset;
    leg.originDomain = origin;
    leg.destinationDomain = destination;
    leg.sourceVault = sourceVault;
    leg.releaseVault = releaseVault;
    leg.operatorId = operatorId;
    leg.window = window;
    leg.amount = amount;
    leg.feeRatio = Ratio::bps(feeBps);
    leg.epoch = epoch;
    leg.hopIndex = hopIndex;
    return leg;
}

void ScenarioBuilder::buildBalanced(ScenarioState& state) const {
    submitOrder(
        state,
        OrderId("ord-bal-001"),
        AccountId("alice"),
        AccountId("beneficiary-beta"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        VaultId("vault-alpha-zusd-a"),
        Amount::of(2'500'000'000),
        EpochId(1));
    submitOrder(
        state,
        OrderId("ord-bal-002"),
        AccountId("bob"),
        AccountId("beneficiary-beta"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        VaultId("vault-alpha-zusd-a"),
        Amount::of(1'750'000'000),
        EpochId(1));

    RoutePlan route;
    route.id = RouteId("route-balanced-alpha-beta");
    route.label = "alpha beta ZUSD fast settlement";
    route.legs.push_back(makeLeg(
        "leg-bal-001",
        OrderId("ord-bal-001"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        DomainId("beta"),
        VaultId("vault-alpha-zusd-a"),
        VaultId("vault-beta-zusd-a"),
        OperatorId("op-atlas"),
        WindowId("w-fast"),
        Amount::of(2'500'000'000),
        9,
        EpochId(1),
        0));
    route.legs.push_back(makeLeg(
        "leg-bal-002",
        OrderId("ord-bal-002"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        DomainId("beta"),
        VaultId("vault-alpha-zusd-a"),
        VaultId("vault-beta-zusd-a"),
        OperatorId("op-atlas"),
        WindowId("w-fast"),
        Amount::of(1'750'000'000),
        9,
        EpochId(1),
        1));
    state.routes.addRoute(route, &state.events);
}

void ScenarioBuilder::buildMultiEpoch(ScenarioState& state) const {
    submitOrder(
        state,
        OrderId("ord-me-001"),
        AccountId("alice"),
        AccountId("beneficiary-beta"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        VaultId("vault-alpha-zusd-a"),
        Amount::of(3'000'000'000),
        EpochId(1));
    submitOrder(
        state,
        OrderId("ord-me-002"),
        AccountId("carol"),
        AccountId("beneficiary-gamma"),
        AssetId("ZUSD"),
        DomainId("beta"),
        VaultId("vault-beta-zusd-a"),
        Amount::of(2'000'000'000),
        EpochId(2));
    submitOrder(
        state,
        OrderId("ord-me-003"),
        AccountId("dan"),
        AccountId("beneficiary-gamma"),
        AssetId("ZUSD"),
        DomainId("beta"),
        VaultId("vault-beta-zusd-a"),
        Amount::of(1'250'000'000),
        EpochId(2));
    submitOrder(
        state,
        OrderId("ord-me-004"),
        AccountId("erin"),
        AccountId("beneficiary-delta"),
        AssetId("ZUSD"),
        DomainId("gamma"),
        VaultId("vault-gamma-zusd-a"),
        Amount::of(1'000'000'000),
        EpochId(3));

    RoutePlan first;
    first.id = RouteId("route-me-alpha-beta");
    first.label = "alpha beta epoch one";
    first.legs.push_back(makeLeg(
        "leg-me-001",
        OrderId("ord-me-001"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        DomainId("beta"),
        VaultId("vault-alpha-zusd-a"),
        VaultId("vault-beta-zusd-a"),
        OperatorId("op-atlas"),
        WindowId("w-fast"),
        Amount::of(3'000'000'000),
        8,
        EpochId(1),
        0));
    state.routes.addRoute(first, &state.events);

    RoutePlan second;
    second.id = RouteId("route-me-beta-gamma-delta");
    second.label = "rolling beta gamma delta";
    second.legs.push_back(makeLeg(
        "leg-me-002",
        OrderId("ord-me-002"),
        AssetId("ZUSD"),
        DomainId("beta"),
        DomainId("gamma"),
        VaultId("vault-beta-zusd-a"),
        VaultId("vault-gamma-zusd-a"),
        OperatorId("op-nova"),
        WindowId("w-roll"),
        Amount::of(2'000'000'000),
        12,
        EpochId(2),
        0));
    second.legs.push_back(makeLeg(
        "leg-me-003",
        OrderId("ord-me-003"),
        AssetId("ZUSD"),
        DomainId("beta"),
        DomainId("gamma"),
        VaultId("vault-beta-zusd-a"),
        VaultId("vault-gamma-zusd-a"),
        OperatorId("op-nova"),
        WindowId("w-roll"),
        Amount::of(1'250'000'000),
        12,
        EpochId(2),
        1));
    second.legs.push_back(makeLeg(
        "leg-me-004",
        OrderId("ord-me-004"),
        AssetId("ZUSD"),
        DomainId("gamma"),
        DomainId("delta"),
        VaultId("vault-gamma-zusd-a"),
        VaultId("vault-delta-zusd-a"),
        OperatorId("op-nova"),
        WindowId("w-roll"),
        Amount::of(1'000'000'000),
        12,
        EpochId(3),
        2));
    state.routes.addRoute(second, &state.events);
}

void ScenarioBuilder::buildOperatorWindows(ScenarioState& state) const {
    submitOrder(
        state,
        OrderId("ord-ow-001"),
        AccountId("alice"),
        AccountId("beneficiary-beta"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        VaultId("vault-alpha-zusd-b"),
        Amount::of(2'200'000'000),
        EpochId(2));
    submitOrder(
        state,
        OrderId("ord-ow-002"),
        AccountId("frank"),
        AccountId("beneficiary-delta"),
        AssetId("ZUSD"),
        DomainId("beta"),
        VaultId("vault-beta-zusd-a"),
        Amount::of(1'400'000'000),
        EpochId(2));
    submitOrder(
        state,
        OrderId("ord-ow-003"),
        AccountId("bob"),
        AccountId("beneficiary-beta"),
        AssetId("ZBTC"),
        DomainId("alpha"),
        VaultId("vault-alpha-zbtc-a"),
        Amount::of(1'200'000),
        EpochId(2));

    RoutePlan usd;
    usd.id = RouteId("route-ow-usd");
    usd.label = "operator split ZUSD";
    usd.legs.push_back(makeLeg(
        "leg-ow-001",
        OrderId("ord-ow-001"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        DomainId("beta"),
        VaultId("vault-alpha-zusd-b"),
        VaultId("vault-beta-zusd-b"),
        OperatorId("op-prism"),
        WindowId("w-deep"),
        Amount::of(2'200'000'000),
        10,
        EpochId(2),
        0));
    usd.legs.push_back(makeLeg(
        "leg-ow-002",
        OrderId("ord-ow-002"),
        AssetId("ZUSD"),
        DomainId("beta"),
        DomainId("delta"),
        VaultId("vault-beta-zusd-a"),
        VaultId("vault-delta-zusd-a"),
        OperatorId("op-nova"),
        WindowId("w-roll"),
        Amount::of(1'400'000'000),
        14,
        EpochId(2),
        1));
    state.routes.addRoute(usd, &state.events);

    RoutePlan btc;
    btc.id = RouteId("route-ow-btc");
    btc.label = "operator split ZBTC";
    btc.legs.push_back(makeLeg(
        "leg-ow-003",
        OrderId("ord-ow-003"),
        AssetId("ZBTC"),
        DomainId("alpha"),
        DomainId("beta"),
        VaultId("vault-alpha-zbtc-a"),
        VaultId("vault-beta-zbtc-a"),
        OperatorId("op-atlas"),
        WindowId("w-fast"),
        Amount::of(1'200'000),
        6,
        EpochId(2),
        0));
    state.routes.addRoute(btc, &state.events);
}

void ScenarioBuilder::buildMeshReconcile(ScenarioState& state) const {
    submitOrder(
        state,
        OrderId("ord-mr-001"),
        AccountId("alice"),
        AccountId("beneficiary-beta"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        VaultId("vault-alpha-zusd-a"),
        Amount::of(1'900'000'000),
        EpochId(2));
    submitOrder(
        state,
        OrderId("ord-mr-002"),
        AccountId("bob"),
        AccountId("beneficiary-beta"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        VaultId("vault-alpha-zusd-b"),
        Amount::of(1'600'000'000),
        EpochId(2));
    submitOrder(
        state,
        OrderId("ord-mr-003"),
        AccountId("carol"),
        AccountId("beneficiary-gamma"),
        AssetId("ZUSD"),
        DomainId("beta"),
        VaultId("vault-beta-zusd-a"),
        Amount::of(1'800'000'000),
        EpochId(2));
    submitOrder(
        state,
        OrderId("ord-mr-004"),
        AccountId("dan"),
        AccountId("beneficiary-gamma"),
        AssetId("ZUSD"),
        DomainId("beta"),
        VaultId("vault-beta-zusd-a"),
        Amount::of(1'100'000'000),
        EpochId(3));
    submitOrder(
        state,
        OrderId("ord-mr-005"),
        AccountId("erin"),
        AccountId("beneficiary-delta"),
        AssetId("ZUSD"),
        DomainId("gamma"),
        VaultId("vault-gamma-zusd-a"),
        Amount::of(900'000'000),
        EpochId(3));

    RoutePlan alphaBeta;
    alphaBeta.id = RouteId("route-mr-alpha-beta");
    alphaBeta.label = "mesh alpha beta";
    alphaBeta.legs.push_back(makeLeg(
        "leg-mr-001",
        OrderId("ord-mr-001"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        DomainId("beta"),
        VaultId("vault-alpha-zusd-a"),
        VaultId("vault-beta-zusd-a"),
        OperatorId("op-prism"),
        WindowId("w-deep"),
        Amount::of(1'900'000'000),
        11,
        EpochId(2),
        0));
    alphaBeta.legs.push_back(makeLeg(
        "leg-mr-002",
        OrderId("ord-mr-002"),
        AssetId("ZUSD"),
        DomainId("alpha"),
        DomainId("beta"),
        VaultId("vault-alpha-zusd-b"),
        VaultId("vault-beta-zusd-b"),
        OperatorId("op-prism"),
        WindowId("w-deep"),
        Amount::of(1'600'000'000),
        11,
        EpochId(2),
        1));
    state.routes.addRoute(alphaBeta, &state.events);

    RoutePlan betaGamma;
    betaGamma.id = RouteId("route-mr-beta-gamma");
    betaGamma.label = "mesh beta gamma";
    betaGamma.legs.push_back(makeLeg(
        "leg-mr-003",
        OrderId("ord-mr-003"),
        AssetId("ZUSD"),
        DomainId("beta"),
        DomainId("gamma"),
        VaultId("vault-beta-zusd-a"),
        VaultId("vault-gamma-zusd-a"),
        OperatorId("op-nova"),
        WindowId("w-roll"),
        Amount::of(1'800'000'000),
        13,
        EpochId(2),
        0));
    betaGamma.legs.push_back(makeLeg(
        "leg-mr-004",
        OrderId("ord-mr-004"),
        AssetId("ZUSD"),
        DomainId("beta"),
        DomainId("gamma"),
        VaultId("vault-beta-zusd-a"),
        VaultId("vault-gamma-zusd-a"),
        OperatorId("op-nova"),
        WindowId("w-roll"),
        Amount::of(1'100'000'000),
        13,
        EpochId(3),
        1));
    state.routes.addRoute(betaGamma, &state.events);

    RoutePlan gammaDelta;
    gammaDelta.id = RouteId("route-mr-gamma-delta");
    gammaDelta.label = "mesh gamma delta";
    gammaDelta.legs.push_back(makeLeg(
        "leg-mr-005",
        OrderId("ord-mr-005"),
        AssetId("ZUSD"),
        DomainId("gamma"),
        DomainId("delta"),
        VaultId("vault-gamma-zusd-a"),
        VaultId("vault-delta-zusd-a"),
        OperatorId("op-prism"),
        WindowId("w-deep"),
        Amount::of(900'000'000),
        11,
        EpochId(3),
        0));
    state.routes.addRoute(gammaDelta, &state.events);
}

void ScenarioBuilder::validateRoutes(const ScenarioState& state) const {
    RouteValidator validator(state.registry, state.ledger, state.routes);
    for (const auto& route : state.routes.routes()) {
        validator.validateRoute(route);
    }
}

void ScenarioBuilder::settle(ScenarioState& state, std::optional<EpochId> maxEpoch) const {
    SettlementEngine engine(state.registry, state.ledger, state.routes, state.events);
    if (maxEpoch.has_value()) {
        state.receipts = engine.settleUntil(*maxEpoch);
    } else {
        state.receipts = engine.settleAll();
    }
    Reconciler reconciler(state.registry, state.ledger, state.receipts);
    state.reconciliation = reconciler.build();
    Event event;
    event.type = EventType::ReconciliationFinished;
    event.subject = "scenario:" + state.name;
    event.fields["openLiability"] = state.reconciliation.openLiability.str();
    event.fields["conservationOk"] = state.reconciliation.conservationOk ? "true" : "false";
    state.events.push(event);
}

} // namespace tdtl
