// tests/test_output_formatter.cpp
#include "catch.hpp"
#include "../src/OutputFormatter.h"
#include "../src/FileReader.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace rcpack;
namespace fs = std::filesystem;

// Helpers for temp directories
static fs::path make_temp_dir(const std::string &prefix = "rcpack_test_") {
    fs::path base = fs::temp_directory_path();
    for (int i = 0; i < 100; ++i) {
        auto candidate = base / (prefix + std::to_string(std::rand()));
        if (!fs::exists(candidate)) {
            fs::create_directory(candidate);
            return candidate;
        }
    }
    throw std::runtime_error("Unable to create temp directory");
}

static void remove_dir_recursive(const fs::path &p) {
    try { if (fs::exists(p)) fs::remove_all(p); } catch(...) {}
}

TEST_CASE("OutputFormatter generates tree for nested directories", "[OutputFormatter][generate]") {
    fs::path tmp = make_temp_dir();
    fs::create_directory(tmp / "dirA");
    fs::create_directory(tmp / "dirB");
    std::ofstream(tmp / "file1.txt").put('x');
    std::ofstream(tmp / "dirA" / "file2.txt").put('y');
    std::ofstream(tmp / "dirB" / "file3.txt").put('z');

    std::vector<FileEntry> files = {
        {tmp / "file1.txt"},
        {tmp / "dirA" / "file2.txt"},
        {tmp / "dirB" / "file3.txt"}
    };

    std::vector<FileContent> contents = {
        FileReader().readFile(tmp / "file1.txt"),
        FileReader().readFile(tmp / "dirA" / "file2.txt"),
        FileReader().readFile(tmp / "dirB" / "file3.txt")
    };

    Config cfg;
    cfg.dirsOnly = false;

    GitInfo git;
    git.isRepo = false;

    ScanResult scan;
    scan.files = files;

    std::ostringstream oss;
    OutputFormatter fmt(oss);
    fmt.generate(tmp, cfg, git, scan, contents);

    std::string out = oss.str();

    REQUIRE(out.find("file1.txt") != std::string::npos);
    REQUIRE(out.find("dirA/") != std::string::npos);
    REQUIRE(out.find("dirB/") != std::string::npos);
    REQUIRE(out.find("file2.txt") != std::string::npos);
    REQUIRE(out.find("file3.txt") != std::string::npos);

    remove_dir_recursive(tmp);
}

TEST_CASE("OutputFormatter generates correctly with Git info", "[OutputFormatter][generate][git]") {
    fs::path tmp = make_temp_dir();
    std::ofstream(tmp / "a.txt") << "hello\nworld\n";

    std::vector<FileEntry> files = {{tmp / "a.txt"}};
    std::vector<FileContent> contents = {FileReader().readFile(tmp / "a.txt")};

    Config cfg;
    cfg.dirsOnly = false;

    GitInfo git;
    git.isRepo = true;
    git.commitSHA = "abc123";
    git.branch = "main";
    git.author = "Tester";
    git.date = "2025-11-13";

    ScanResult scan;
    scan.files = files;

    std::ostringstream oss;
    OutputFormatter fmt(oss);
    fmt.generate(tmp, cfg, git, scan, contents);

    std::string out = oss.str();
    REQUIRE(out.find("## Git Info") != std::string::npos);
    REQUIRE(out.find("Commit: abc123") != std::string::npos);
    REQUIRE(out.find("Branch: main") != std::string::npos);
    REQUIRE(out.find("Author: Tester") != std::string::npos);
    REQUIRE(out.find("Date: 2025-11-13") != std::string::npos);

    REQUIRE(out.find("### File: a.txt") != std::string::npos);
    REQUIRE(out.find("hello") != std::string::npos);
    REQUIRE(out.find("world") != std::string::npos);

    REQUIRE(out.find("## Summary") != std::string::npos);
    REQUIRE(out.find("Total files: 1") != std::string::npos);
    REQUIRE(out.find("Total lines: 2") != std::string::npos);

    remove_dir_recursive(tmp);
}

TEST_CASE("OutputFormatter handles directory-only mode", "[OutputFormatter][generate][dirsOnly]") {
    fs::path tmp = make_temp_dir();
    std::ofstream(tmp / "b.txt") << "some content\n";

    std::vector<FileEntry> files = {{tmp / "b.txt"}};
    std::vector<FileContent> contents; // ignored in dirsOnly mode

    Config cfg;
    cfg.dirsOnly = true;

    GitInfo git;
    git.isRepo = false;

    ScanResult scan;
    scan.files = files;

    std::ostringstream oss;
    OutputFormatter fmt(oss);
    fmt.generate(tmp, cfg, git, scan, contents);

    std::string out = oss.str();
    REQUIRE(out.find("(skipped: directory-only mode)") != std::string::npos);
    REQUIRE(out.find("## Git Info") != std::string::npos);
    REQUIRE(out.find("Not a git repository") != std::string::npos);

    remove_dir_recursive(tmp);
}

TEST_CASE("OutputFormatter handles files outside root directory", "[OutputFormatter][generate][outside]") {
    fs::path tmp = make_temp_dir();
    fs::path outside = fs::temp_directory_path() / ("outside_" + std::to_string(std::rand()));
    std::ofstream(outside) << "hi\n";

    std::vector<FileEntry> files = {{outside}};
    std::vector<FileContent> contents = {FileReader().readFile(outside)};

    Config cfg;
    cfg.dirsOnly = false;

    GitInfo git;
    git.isRepo = false;

    ScanResult scan;
    scan.files = files;

    std::ostringstream oss;
    OutputFormatter fmt(oss);
    fmt.generate(tmp, cfg, git, scan, contents);

    std::string out = oss.str();
    REQUIRE(out.find(outside.filename().string()) != std::string::npos);

    remove_dir_recursive(tmp);
    std::filesystem::remove(outside);
}
