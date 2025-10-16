#include <iostream>
#include "RepositoryScanner.h"

namespace fs = std::filesystem;
using namespace rcpack;

//Optional Functionality -i or --include:
RepositoryScanner::RepositoryScanner(std::vector<std::string> includePatterns, std::vector<std::string> excludePatterns) {
    // normalize patterns: accept "*.js" or ".js" or "js"
    for (auto &pat : includePatterns) {
        std::string ext = normalize_extension_token(pat);
        if (!ext.empty()) {
            rs_patterns.push_back(ext); // ".js"
        } else {
            std::cerr << "Warning: include pattern '" << pat << "' is not an extension form and will be ignored.\n";
        }
    }

    // compile exclude regexes (user-supplied regex strings)
    for (auto &raw : excludePatterns) {
        std::string s = trim(raw);
        if (s.empty()) continue;
        try {
            rs_excludeRegexes.emplace_back(s, std::regex::ECMAScript | std::regex::icase);
        } catch (const std::regex_error &e) {
            std::cerr << "Warning: invalid exclude regex '" << raw << "' -> " << e.what() << "\n";
        } catch (const std::exception &e) {
            std::cerr << "Warning: failed to compile exclude regex '" << raw << "' -> " << e.what() << "\n";
        }
    }
}

bool RepositoryScanner::matches(const fs::path &p) const {
    // First, check exclude regexes: if any matches the file path, treat as excluded.
    if (!rs_excludeRegexes.empty()) {
        // Use generic_string() to get platform-neutral separators (forward slashes)
        std::string pathStr = p.generic_string();
        for (const auto &re : rs_excludeRegexes) {
            try {
                if (std::regex_search(pathStr, re)) {
                    return false; // excluded
                }
            } catch (const std::regex_error &e) {
                // Should not normally happen because we compiled earlier, but be safe.
                std::cerr << "Warning: regex_search failed on '" << pathStr << "' -> " << e.what() << "\n";
            }
        }
    }

    // If no include patterns were provided, accept everything (unless excluded above).
    if (rs_patterns.empty()) return true;

    // Get file extension (includes the leading dot, e.g. ".cpp") and compare.
    std::string ext = p.has_extension() ? toLower(p.extension().string()) : std::string();
    for (const auto &pat : rs_patterns) {
        if (ext == pat) return true;
    }
    return false;
}

//main function that walks through directories/files
ScanResult RepositoryScanner::scanPaths(const std::vector<std::string>& paths){
    ScanResult result;
    for(const auto &pstr : paths){
        fs::path p(pstr);
        try {
            //skip non-existing paths and log a warning
            if (!fs::exists(p)){
                std::cerr << "Warning: path does not exist: " << p << "\n";
                result.skipped.push_back(p);
                continue;
            }

            if (fs::is_regular_file(p)) {
                if (matches(p)) {
                    std::error_code ec;
                    uintmax_t sz = fs::file_size(p, ec);
                    if (ec) {
                        std::cerr << "Warning (file_size): " << p << " -> " << ec.message() << "\n";
                        result.skipped.push_back(p);
                    } else {
                        FileEntry e{p, sz};
                        result.files.push_back(std::move(e));
                    }
                }
            } else if (fs::is_directory(p)){
                fs::recursive_directory_iterator start(p, fs::directory_options::skip_permission_denied);
                fs::recursive_directory_iterator end; //default-constructed iterator that represents the “end” of a recursive directory traversal.
                for (auto it = start; it != end; ++it){
                    try {
                        const fs::path entryPath = it->path();
                        if (fs::is_regular_file(entryPath) && matches(entryPath)) {
                            std::error_code ec;
                            uintmax_t sz = fs::file_size(entryPath, ec);
                            if (ec) {
                                std::cerr << "Warning (file_size): " << entryPath << " -> " << ec.message() << "\n";
                                result.skipped.push_back(entryPath);
                            } else {
                                FileEntry e{entryPath, sz};
                                result.files.push_back(std::move(e));
                            }
                        }
                    } catch (const std::exception &ex){
                        std::cerr << "Warning (file): " << it->path() << " -> " << ex.what() << "\n";
                        result.skipped.push_back(it->path());
                    }
                }
            } else {
                std::cerr << "Skipping special file: " << p << "\n";
                result.skipped.push_back(p);
            }
        } catch (const std::exception &ex){
            std::cerr << "Warning (path): " << p << " -> " << ex.what() << "\n";
            result.skipped.push_back(p);
        }
    }
    std::sort(result.files.begin(), result.files.end(),
              [](const FileEntry &a, const FileEntry &b){
                  return a.path.generic_string() < b.path.generic_string();
              });

    return result;
}