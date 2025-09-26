#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <regex>
#include <optional>
#include "utils.h"

namespace rcpack {

    struct FileEntry {
        std::filesystem::path path;
        uintmax_t size = 0;
    };

    struct ScanResult {
        std::vector<FileEntry> files;
        std::vector<std::filesystem::path> skipped; // unreadable or wrong
    };

    class RepositoryScanner {
        std::vector<std::string> rs_patterns;
        std::vector<std::regex> rs_excludeRegexes;
        bool matches(const std::filesystem::path &p) const;
    public:
        RepositoryScanner(std::vector<std::string> includePatterns = {}, std::vector<std::string> excludePatterns = {});
        ScanResult scanPaths(const std::vector<std::string>& paths);
    };
}