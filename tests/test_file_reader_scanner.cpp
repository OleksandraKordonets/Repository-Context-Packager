// tests/test_file_reader_scanner.cpp
#include "catch.hpp"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>

#include "../src/FileReader.h"
#include "../src/RepositoryScanner.h"
#include "../src/utils.h"

using namespace rcpack;
namespace fs = std::filesystem;

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
    try {
        if (fs::exists(p)) fs::remove_all(p);
    } catch(...) { /* best-effort cleanup */ }
}

TEST_CASE("FileReader: full read small file and lines count", "[FileReader][readFile]") {
    fs::path tmp = make_temp_dir();
    fs::path f = tmp / "small.txt";

    std::ofstream ofs(f.string(), std::ios::binary);
    ofs << "line1\nline2\nlast\n";
    ofs.close();

    FileReader r;
    auto fc = r.readFile(f);

    REQUIRE(fc.truncated == false);
    REQUIRE(fc.lines == 3);
    REQUIRE(fc.content.find("line1") != std::string::npos);

    remove_dir_recursive(tmp);
}

TEST_CASE("FileReader: truncated read when file larger than maxBytes", "[FileReader][truncated]") {
    fs::path tmp = make_temp_dir();
    fs::path f = tmp / "big.bin";

    std::ofstream ofs(f.string(), std::ios::binary);
    for (int i = 0; i < 100; ++i) ofs.put('A' + (i % 26));
    ofs.close();

    FileReader r(10);

    auto fc = r.readFile(f);

    REQUIRE(fc.truncated == true);
    REQUIRE(fc.content.size() <= 10);
    REQUIRE(fc.lines == std::count(fc.content.begin(), fc.content.end(), '\n'));

    remove_dir_recursive(tmp);
}

TEST_CASE("FileReader: non-existent file returns empty FileContent", "[FileReader][error]") {
    fs::path tmp = make_temp_dir();
    fs::path f = tmp / "does_not_exist.txt";

    FileReader r;
    auto fc = r.readFile(f);

    REQUIRE(fc.content.empty());
    REQUIRE(fc.truncated == false);

    remove_dir_recursive(tmp);
}

TEST_CASE("RepositoryScanner: include pattern filters by extension", "[RepositoryScanner][include]") {
    fs::path tmp = make_temp_dir();
    fs::path dir = tmp / "proj";
    fs::create_directory(dir);

    std::ofstream(dir / "a.cpp").put('x');
    std::ofstream(dir / "b.txt").put('y');
    std::ofstream(dir / "c.h").put('z');

    RepositoryScanner scanner(/*includePatterns=*/{ "*.cpp" }, /*excludePatterns=*/{});
    auto result = scanner.scanPaths({ dir.string() });

    std::vector<std::string> names;
    for (auto &e : result.files) names.push_back(e.path.filename().string());
    REQUIRE(names.size() == 1);
    REQUIRE(names[0] == "a.cpp");

    remove_dir_recursive(tmp);
}

TEST_CASE("RepositoryScanner: no include patterns -> include all regular files", "[RepositoryScanner][include-none]") {
    fs::path tmp = make_temp_dir();
    fs::path dir = tmp / "proj2";
    fs::create_directory(dir);

    std::ofstream(dir / "one.py").put('x');
    std::ofstream(dir / "two.md").put('y');

    RepositoryScanner scanner({}, {});
    auto result = scanner.scanPaths({ dir.string() });

    std::vector<std::string> names;
    for (auto &e : result.files) names.push_back(e.path.filename().string());
    std::sort(names.begin(), names.end());
    REQUIRE(names.size() == 2);
    REQUIRE(std::find(names.begin(), names.end(), "one.py") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "two.md") != names.end());

    remove_dir_recursive(tmp);
}

TEST_CASE("RepositoryScanner: exclude regex filters files", "[RepositoryScanner][exclude]") {
    fs::path tmp = make_temp_dir();
    fs::path dir = tmp / "proj3";
    fs::create_directory(dir);

    std::ofstream(dir / "public.cpp").put('x');
    std::ofstream(dir / "secret_password.txt").put('y');
    std::ofstream(dir / "README.md").put('z');

    RepositoryScanner scanner({}, { ".*secret.*" });
    auto result = scanner.scanPaths({ dir.string() });

    std::vector<std::string> names;
    for (auto &e : result.files) names.push_back(e.path.filename().string());
    REQUIRE(std::find(names.begin(), names.end(), "secret_password.txt") == names.end());
    REQUIRE(std::find(names.begin(), names.end(), "public.cpp") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "README.md") != names.end());

    remove_dir_recursive(tmp);
}

TEST_CASE("RepositoryScanner: scanPaths handles non-existing input path by adding to skipped", "[RepositoryScanner][skipped]") {
    fs::path not_exist = fs::temp_directory_path() / ("totally_missing_" + std::to_string(std::rand()));

    RepositoryScanner scanner({}, {});
    auto result = scanner.scanPaths({ not_exist.string() });

    bool found = false;
    for (auto &p : result.skipped) {
        if (p.generic_string() == not_exist.generic_string()) { found = true; break; }
    }
    REQUIRE(found == true);
}
