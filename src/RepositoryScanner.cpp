#include <iostream>
#include <regex>
#include "utils.h"
#include "RepositoryScanner.h"

namespace fs = std::filesystem;
using namespace rcpack;

//Optional Functionality -i or --include:
RepositoryScanner::RepositoryScanner(std::vector<std::string> includePatterns){
    // normalize patterns: accept "*.js" or ".js" or "js"
    for(auto &pat : includePatterns){
        std::string p = pat;
        // trim
        p = trim(p);
        if (p.size()>=2 && p[0]=='*' && p[1]=='.'){
            p = p.substr(1); // ".js"
        }
        if (p.size()>0 && p[0]=='.'){
            // keep ".js"
            rs_patterns.push_back(toLower(p));
        } else if (!p.empty()){
            // maybe user passed "js" or "*.js"
            rs_patterns.push_back("." + toLower(p));
        }
    }
}

bool RepositoryScanner::matches(const fs::path &p) const {
    //If no include patterns were provided, accept everything
    if (rs_patterns.empty()) return true;
    // Get file extension 
    std::string ext = p.has_extension() ? toLower(p.extension().string()) : std::string();
    //compare against normalized patterns
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
            if (fs::is_regular_file(p)){
                if (matches(p)){
                    FileEntry e{p, fs::file_size(p)};
                    result.files.push_back(std::move(e));
                }
            } else if (fs::is_directory(p)){
                fs::recursive_directory_iterator start(p, fs::directory_options::skip_permission_denied);
                fs::recursive_directory_iterator end; //default-constructed iterator that represents the “end” of a recursive directory traversal.
                for (auto it = start; it != end; ++it){
                    try {
                        if (fs::is_regular_file(it->path()) && matches(it->path())){
                            FileEntry e{it->path(), fs::file_size(it->path())};
                            result.files.push_back(std::move(e));
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
    return result;
}