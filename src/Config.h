#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>

class Config {
public:
    std::vector<std::string> c_paths{};
    std::string c_outputFile{};
    std::vector<std::string> c_includePatterns{};
    bool showHelp = false;
    bool showVersion = false;
    //bool respectGitignore = true;
};
#endif