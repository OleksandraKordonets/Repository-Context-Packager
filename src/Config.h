#pragma once

#include <string>
#include <vector>

namespace rcpack {
    
    class Config {
    public:
        std::vector<std::string> c_paths{};
        std::string c_outputFile{};
        std::vector<std::string> c_includePatterns{};
        std::vector<std::string> c_excludePatterns{};
        bool showHelp = false;
        bool showVersion = false;
        bool showRecent = false;
        bool dirsOnly = false;
    };
}