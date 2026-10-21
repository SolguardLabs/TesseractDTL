#include "errors.hpp"
#include "report.hpp"
#include "scenario.hpp"
#include "treasury.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace {

struct Args {
    std::string command;
    std::string scenario;
    bool json{false};
    bool events{false};
    std::optional<tdtl::EpochId> epoch;
};

void usage(std::ostream& out) {
    out << "TesseractDTL CLI\n";
    out << "\n";
    out << "Commands:\n";
    out << "  list [--json]\n";
    out << "  validate <scenario>\n";
    out << "  inspect <scenario> [--epoch N]\n";
    out << "  run <scenario> [--json] [--events] [--epoch N]\n";
    out << "  treasury <scenario> [--epoch N]\n";
    out << "\n";
    out << "Scenarios are built in and deterministic.\n";
}

Args parse(int argc, char** argv) {
    Args args;
    if (argc < 2) {
        args.command = "help";
        return args;
    }
    args.command = argv[1];
    int index = 2;
    if (args.command == "run" || args.command == "validate" || args.command == "inspect" || args.command == "treasury") {
        if (index >= argc) {
            throw tdtl::ScenarioError("missing scenario name");
        }
        args.scenario = argv[index++];
    }
    while (index < argc) {
        std::string token = argv[index++];
        if (token == "--json") {
            args.json = true;
        } else if (token == "--events") {
            args.events = true;
        } else if (token == "--epoch") {
            if (index >= argc) {
                throw tdtl::ScenarioError("missing epoch value");
            }
            args.epoch = tdtl::EpochId(std::stoi(argv[index++]));
        } else {
            throw tdtl::ScenarioError("unknown argument " + token);
        }
    }
    return args;
}

int run(const Args& args) {
    tdtl::ScenarioBuilder builder;
    if (args.command == "help" || args.command == "--help" || args.command == "-h") {
        usage(std::cout);
        return 0;
    }
    if (args.command == "list") {
        tdtl::ReportWriter::writeScenarioList(std::cout, builder.list(), args.json);
        return 0;
    }
    if (args.command == "validate") {
        const auto state = builder.build(args.scenario, args.epoch);
        (void)state;
        std::cout << "ok " << args.scenario << "\n";
        return 0;
    }
    if (args.command == "inspect") {
        const auto state = builder.build(args.scenario, args.epoch);
        tdtl::ReportWriter::writeText(std::cout, state);
        return 0;
    }
    if (args.command == "treasury") {
        const auto state = builder.build(args.scenario, args.epoch);
        tdtl::TreasuryAnalyzer::writeJson(std::cout, state);
        return 0;
    }
    if (args.command == "run") {
        const auto state = builder.build(args.scenario, args.epoch);
        if (args.json) {
            tdtl::ReportWriter::writeJson(std::cout, state, args.events);
        } else {
            tdtl::ReportWriter::writeText(std::cout, state);
        }
        return 0;
    }
    throw tdtl::ScenarioError("unknown command " + args.command);
}

} // namespace

int main(int argc, char** argv) {
    try {
        const Args args = parse(argc, argv);
        return run(args);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
}
