// tests/test_cli.cpp
#include "catch.hpp"

#include "../src/cli.h"
#include "../src/Config.h"

#include <vector>
#include <string>

using namespace rcpack;

static std::vector<char*> make_argv(const std::vector<std::string>& args) {
    // returns vector<char*> where lifetime is tied to returned vector contents
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (auto &s : args) argv.push_back(const_cast<char*>(s.c_str()));
    argv.push_back(nullptr);
    return argv;
}

TEST_CASE("CLI::parse recognizes flags and options", "[CLI][parse]") {
    std::vector<std::string> raw = {
        "repo-context-packager",
        "-o", "out.txt",
        "-i", "*.cpp,*.h",
        "-ep", ".*secret.*",
        "-c",
        "-d",
        "--remove-comments"
    };
    auto argvvec = make_argv(raw);
    int argc = static_cast<int>(raw.size());
    char** argv = argvvec.data();

    rcpack::CLI cli(argc, argv);
    Config cfg = cli.parse();

    REQUIRE(cfg.c_outputFile == "out.txt");
    REQUIRE(cfg.c_includePatterns.size() == 2); // split_comma should produce two patterns
    REQUIRE( (cfg.c_includePatterns[0] == "*.cpp" || cfg.c_includePatterns[1] == "*.cpp") );
    REQUIRE(cfg.c_excludePatterns.size() == 1);
    REQUIRE(cfg.compress == true);
    REQUIRE(cfg.dirsOnly == true);
    REQUIRE(cfg.removeComments == true);
}

TEST_CASE("CLI::parse defaults path to '.' when none provided", "[CLI][parse][defaults]") {
    std::vector<std::string> raw = { "repo-context-packager" };
    auto argvvec = make_argv(raw);
    int argc = static_cast<int>(raw.size());
    char** argv = argvvec.data();

    rcpack::CLI cli(argc, argv);
    Config cfg = cli.parse();

    REQUIRE(cfg.c_paths.size() == 1);
    REQUIRE(cfg.c_paths[0] == ".");
}
