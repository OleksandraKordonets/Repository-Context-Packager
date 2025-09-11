#pragma once

#include <string>
#include <vector>

namespace rcpack {
    
    class Config {
    public:
        std::vector<std::string> c_paths{};
        std::string c_outputFile{};
        std::vector<std::string> c_includePatterns{};
        bool showHelp = false;
        bool showVersion = false;
    };
}