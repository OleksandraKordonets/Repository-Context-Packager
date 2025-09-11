#pragma once
//RepositoryScanner is responsible for discovering all the files in the repository
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

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
        bool matches(const std::filesystem::path &p) const;
    public:
        RepositoryScanner(std::vector<std::string> includePatterns = {});
        ScanResult scanPaths(const std::vector<std::string>& paths);
    };
}