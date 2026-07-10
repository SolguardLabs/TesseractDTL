#include "report.hpp"

#include "event.hpp"

#include <iomanip>
#include <iostream>

namespace tdtl {

JsonStream::JsonStream(std::ostream& out) : out_(out) {}

void JsonStream::objectBegin() {
    out_ << "{";
}

void JsonStream::objectEnd() {
    out_ << "}";
}

void JsonStream::arrayBegin() {
    out_ << "[";
}

void JsonStream::arrayEnd() {
    out_ << "]";
}

void JsonStream::key(const std::string& value) {
    string(value);
    out_ << ":";
}

void JsonStream::comma() {
    out_ << ",";
}

void JsonStream::string(const std::string& value) {
    out_ << '"';
    out_ << escape(value);
    out_ << '"';
}

void JsonStream::number(std::int64_t value) {
    out_ << value;
}

void JsonStream::boolean(bool value) {
    out_ << (value ? "true" : "false");
}

void JsonStream::raw(const std::string& value) {
    out_ << value;
}

std::string JsonStream::escape(const std::string& value) {
    std::string out;
    for (char c : value) {
        switch (c) {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            out.push_back(c);
            break;
        }
    }
    return out;
}

void ReportWriter::writeJson(std::ostream& out, const ScenarioState& state, bool includeEvents) {
    JsonStream json(out);
    json.objectBegin();
    json.key("protocol");
    json.string("TesseractDTL");
    json.comma();
    json.key("scenario");
    json.string(state.name);
    json.comma();
    json.key("summary");
    writeSummary(json, state);
    json.comma();
    json.key("registry");
    writeRegistry(json, state);
    json.comma();
    json.key("accounts");
    writeAccounts(json, state);
    json.comma();
    json.key("vaults");
    writeVaults(json, state);
    json.comma();
    json.key("orders");
    writeOrders(json, state);
    json.comma();
    json.key("routes");
    writeRoutes(json, state);
    json.comma();
    json.key("epochs");
    writeEpochs(json, state);
    json.comma();
    json.key("reconciliation");
    writeReconciliation(json, state);
    if (includeEvents) {
        json.comma();
        json.key("events");
        writeEvents(json, state);
    }
    json.objectEnd();
    out << "\n";
}

void ReportWriter::writeText(std::ostream& out, const ScenarioState& state) {
    out << "TesseractDTL scenario: " << state.name << "\n";
    out << "orders: " << state.routes.orders().size() << "\n";
    out << "routes: " << state.routes.routes().size() << "\n";
    out << "epochs settled: " << state.receipts.size() << "\n";
    out << "open liability: " << state.reconciliation.openLiability << "\n";
    out << "conservation: " << (state.reconciliation.conservationOk ? "ok" : "failed") << "\n";
    out << "vaults:\n";
    for (const auto& vault : state.ledger.vaults()) {
        out << "  - " << vault.id.str();
        out << " asset=" << vault.asset.str();
        out << " domain=" << vault.domain.str();
        out << " cash=" << vault.cash;
        out << " pending=" << vault.pendingLiability;
        out << " released=" << vault.released;
        out << "\n";
    }
}

void ReportWriter::writeScenarioList(
    std::ostream& out,
    const std::vector<ScenarioInfo>& scenarios,
    bool jsonOutput) {
    if (!jsonOutput) {
        for (const auto& scenario : scenarios) {
            out << scenario.name << " - " << scenario.title << "\n";
        }
        return;
    }
    JsonStream json(out);
    json.arrayBegin();
    for (std::size_t i = 0; i < scenarios.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& scenario = scenarios[i];
        json.objectBegin();
        json.key("name");
        json.string(scenario.name);
        json.comma();
        json.key("title");
        json.string(scenario.title);
        json.comma();
        json.key("description");
        json.string(scenario.description);
        json.comma();
        json.key("expectedOrders");
        json.number(scenario.expectedOrders);
        json.comma();
        json.key("expectedRoutes");
        json.number(scenario.expectedRoutes);
        json.objectEnd();
    }
    json.arrayEnd();
    out << "\n";
}

void ReportWriter::writeSummary(JsonStream& json, const ScenarioState& state) {
    Amount gross = Amount::zero();
    Amount fee = Amount::zero();
    Amount net = Amount::zero();
    int instructionCount = 0;
    for (const auto& epoch : state.receipts) {
        gross += epoch.gross;
        fee += epoch.fee;
        net += epoch.net;
        instructionCount += static_cast<int>(epoch.instructions.size());
    }
    json.objectBegin();
    json.key("orders");
    json.number(static_cast<std::int64_t>(state.routes.orders().size()));
    json.comma();
    json.key("routes");
    json.number(static_cast<std::int64_t>(state.routes.routes().size()));
    json.comma();
    json.key("epochsSettled");
    json.number(static_cast<std::int64_t>(state.receipts.size()));
    json.comma();
    json.key("instructions");
    json.number(instructionCount);
    json.comma();
    json.key("gross");
    json.number(gross.raw());
    json.comma();
    json.key("fee");
    json.number(fee.raw());
    json.comma();
    json.key("net");
    json.number(net.raw());
    json.comma();
    json.key("openLiability");
    json.number(state.reconciliation.openLiability.raw());
    json.comma();
    json.key("conservationOk");
    json.boolean(state.reconciliation.conservationOk);
    json.comma();
    json.key("allVaultsHealthy");
    json.boolean(state.reconciliation.allVaultsHealthy);
    json.objectEnd();
}

void ReportWriter::writeRegistry(JsonStream& json, const ScenarioState& state) {
    json.objectBegin();
    json.key("assets");
    json.arrayBegin();
    const auto assets = state.registry.assets();
    for (std::size_t i = 0; i < assets.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& asset = assets[i];
        json.objectBegin();
        json.key("id");
        json.string(asset.id.str());
        json.comma();
        json.key("symbol");
        json.string(asset.symbol);
        json.comma();
        json.key("decimals");
        json.number(asset.decimals);
        json.objectEnd();
    }
    json.arrayEnd();
    json.comma();
    json.key("domains");
    json.arrayBegin();
    const auto domains = state.registry.domains();
    for (std::size_t i = 0; i < domains.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& domain = domains[i];
        json.objectBegin();
        json.key("id");
        json.string(domain.id.str());
        json.comma();
        json.key("name");
        json.string(domain.name);
        json.comma();
        json.key("depth");
        json.number(domain.settlementDepth);
        json.objectEnd();
    }
    json.arrayEnd();
    json.objectEnd();
}

void ReportWriter::writeAccounts(JsonStream& json, const ScenarioState& state) {
    const auto accounts = state.ledger.accounts();
    json.arrayBegin();
    for (std::size_t i = 0; i < accounts.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& account = accounts[i];
        json.objectBegin();
        json.key("account");
        json.string(account.account.str());
        json.comma();
        json.key("asset");
        json.string(account.asset.str());
        json.comma();
        json.key("amount");
        json.number(account.amount.raw());
        json.objectEnd();
    }
    json.arrayEnd();
}

void ReportWriter::writeVaults(JsonStream& json, const ScenarioState& state) {
    const auto vaults = state.ledger.vaults();
    json.arrayBegin();
    for (std::size_t i = 0; i < vaults.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& vault = vaults[i];
        json.objectBegin();
        json.key("id");
        json.string(vault.id.str());
        json.comma();
        json.key("asset");
        json.string(vault.asset.str());
        json.comma();
        json.key("domain");
        json.string(vault.domain.str());
        json.comma();
        json.key("controller");
        json.string(vault.controller.str());
        json.comma();
        json.key("cash");
        json.number(vault.cash.raw());
        json.comma();
        json.key("pending");
        json.number(vault.pendingLiability.raw());
        json.comma();
        json.key("available");
        json.number(vault.available().raw());
        json.comma();
        json.key("released");
        json.number(vault.released.raw());
        json.comma();
        json.key("feesPaid");
        json.number(vault.feesPaid.raw());
        json.comma();
        json.key("deposits");
        json.number(vault.deposits.raw());
        json.comma();
        json.key("surplus");
        json.number(vault.surplus().raw());
        json.objectEnd();
    }
    json.arrayEnd();
}

void ReportWriter::writeOrders(JsonStream& json, const ScenarioState& state) {
    const auto orders = state.routes.orders();
    json.arrayBegin();
    for (std::size_t i = 0; i < orders.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& order = orders[i];
        const auto& liability = state.ledger.liability(order.id);
        json.objectBegin();
        json.key("id");
        json.string(order.id.str());
        json.comma();
        json.key("owner");
        json.string(order.owner.str());
        json.comma();
        json.key("beneficiary");
        json.string(order.beneficiary.str());
        json.comma();
        json.key("asset");
        json.string(order.asset.str());
        json.comma();
        json.key("originDomain");
        json.string(order.originDomain.str());
        json.comma();
        json.key("sourceVault");
        json.string(order.sourceVault.str());
        json.comma();
        json.key("amount");
        json.number(order.amount.raw());
        json.comma();
        json.key("remaining");
        json.number(liability.remaining().raw());
        json.comma();
        json.key("status");
        json.string(orderStatusName(order.status));
        json.objectEnd();
    }
    json.arrayEnd();
}

void ReportWriter::writeRoutes(JsonStream& json, const ScenarioState& state) {
    const auto routes = state.routes.routes();
    json.arrayBegin();
    for (std::size_t i = 0; i < routes.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& route = routes[i];
        json.objectBegin();
        json.key("id");
        json.string(route.id.str());
        json.comma();
        json.key("label");
        json.string(route.label);
        json.comma();
        json.key("amount");
        json.number(route.totalAmount().raw());
        json.comma();
        json.key("fee");
        json.number(route.totalFee().raw());
        json.comma();
        json.key("legs");
        json.arrayBegin();
        for (std::size_t j = 0; j < route.legs.size(); ++j) {
            if (j != 0) {
                json.comma();
            }
            const auto& leg = route.legs[j];
            json.objectBegin();
            json.key("id");
            json.string(leg.legId);
            json.comma();
            json.key("order");
            json.string(leg.order.str());
            json.comma();
            json.key("asset");
            json.string(leg.asset.str());
            json.comma();
            json.key("originDomain");
            json.string(leg.originDomain.str());
            json.comma();
            json.key("destinationDomain");
            json.string(leg.destinationDomain.str());
            json.comma();
            json.key("sourceVault");
            json.string(leg.sourceVault.str());
            json.comma();
            json.key("releaseVault");
            json.string(leg.releaseVault.str());
            json.comma();
            json.key("operator");
            json.string(leg.operatorId.str());
            json.comma();
            json.key("window");
            json.string(leg.window.str());
            json.comma();
            json.key("epoch");
            json.number(leg.epoch.value);
            json.comma();
            json.key("amount");
            json.number(leg.amount.raw());
            json.comma();
            json.key("fee");
            json.number(leg.fee().raw());
            json.objectEnd();
        }
        json.arrayEnd();
        json.objectEnd();
    }
    json.arrayEnd();
}

void ReportWriter::writeEpochs(JsonStream& json, const ScenarioState& state) {
    json.arrayBegin();
    for (std::size_t i = 0; i < state.receipts.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& epoch = state.receipts[i];
        json.objectBegin();
        json.key("epoch");
        json.number(epoch.epoch.value);
        json.comma();
        json.key("settled");
        json.boolean(epoch.settled);
        json.comma();
        json.key("gross");
        json.number(epoch.gross.raw());
        json.comma();
        json.key("fee");
        json.number(epoch.fee.raw());
        json.comma();
        json.key("net");
        json.number(epoch.net.raw());
        json.comma();
        json.key("instructions");
        json.arrayBegin();
        for (std::size_t j = 0; j < epoch.instructions.size(); ++j) {
            if (j != 0) {
                json.comma();
            }
            const auto& instruction = epoch.instructions[j];
            json.objectBegin();
            json.key("key");
            json.string(instruction.key);
            json.comma();
            json.key("releaseVault");
            json.string(instruction.releaseVault.str());
            json.comma();
            json.key("asset");
            json.string(instruction.asset.str());
            json.comma();
            json.key("operator");
            json.string(instruction.operatorId.str());
            json.comma();
            json.key("gross");
            json.number(instruction.gross.raw());
            json.comma();
            json.key("fee");
            json.number(instruction.fee.raw());
            json.comma();
            json.key("net");
            json.number(instruction.net.raw());
            json.comma();
            json.key("obligations");
            json.number(static_cast<std::int64_t>(instruction.obligations));
            json.objectEnd();
        }
        json.arrayEnd();
        json.objectEnd();
    }
    json.arrayEnd();
}

void ReportWriter::writeReconciliation(JsonStream& json, const ScenarioState& state) {
    const auto& report = state.reconciliation;
    json.objectBegin();
    json.key("openLiability");
    json.number(report.openLiability.raw());
    json.comma();
    json.key("conservationOk");
    json.boolean(report.conservationOk);
    json.comma();
    json.key("allVaultsHealthy");
    json.boolean(report.allVaultsHealthy);
    json.comma();
    json.key("assets");
    json.arrayBegin();
    for (std::size_t i = 0; i < report.assets.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& row = report.assets[i];
        json.objectBegin();
        json.key("asset");
        json.string(row.asset.str());
        json.comma();
        json.key("vaultCash");
        json.number(row.vaultCash.raw());
        json.comma();
        json.key("accounts");
        json.number(row.accounts.raw());
        json.comma();
        json.key("pending");
        json.number(row.pending.raw());
        json.comma();
        json.key("released");
        json.number(row.released.raw());
        json.comma();
        json.key("deposits");
        json.number(row.deposits.raw());
        json.comma();
        json.key("supply");
        json.number(row.supply.raw());
        json.comma();
        json.key("conservationOk");
        json.boolean(row.conservationOk);
        json.objectEnd();
    }
    json.arrayEnd();
    json.comma();
    json.key("domains");
    json.arrayBegin();
    for (std::size_t i = 0; i < report.domains.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& row = report.domains[i];
        json.objectBegin();
        json.key("domain");
        json.string(row.domain.str());
        json.comma();
        json.key("asset");
        json.string(row.asset.str());
        json.comma();
        json.key("vaultCash");
        json.number(row.vaultCash.raw());
        json.comma();
        json.key("pending");
        json.number(row.pending.raw());
        json.comma();
        json.key("released");
        json.number(row.released.raw());
        json.comma();
        json.key("vaults");
        json.number(row.vaults);
        json.objectEnd();
    }
    json.arrayEnd();
    json.comma();
    json.key("operators");
    json.arrayBegin();
    for (std::size_t i = 0; i < report.operators.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& row = report.operators[i];
        json.objectBegin();
        json.key("operator");
        json.string(row.operatorId.str());
        json.comma();
        json.key("asset");
        json.string(row.asset.str());
        json.comma();
        json.key("fees");
        json.number(row.fees.raw());
        json.comma();
        json.key("gross");
        json.number(row.gross.raw());
        json.comma();
        json.key("instructions");
        json.number(row.instructions);
        json.objectEnd();
    }
    json.arrayEnd();
    json.objectEnd();
}

void ReportWriter::writeEvents(JsonStream& json, const ScenarioState& state) {
    const auto& events = state.events.all();
    json.arrayBegin();
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (i != 0) {
            json.comma();
        }
        const auto& event = events[i];
        json.objectBegin();
        json.key("type");
        json.string(eventTypeName(event.type));
        json.comma();
        json.key("epoch");
        json.number(event.epoch.value);
        json.comma();
        json.key("subject");
        json.string(event.subject);
        json.comma();
        json.key("fields");
        json.objectBegin();
        std::size_t seen = 0;
        for (const auto& field : event.fields) {
            if (seen != 0) {
                json.comma();
            }
            json.key(field.first);
            json.string(field.second);
            ++seen;
        }
        json.objectEnd();
        json.objectEnd();
    }
    json.arrayEnd();
}

} // namespace tdtl
