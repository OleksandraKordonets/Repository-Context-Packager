#include "cli.h"
#include <iostream>
#include <sstream>
#include <algorithm>

CLI::CLI(int argc, char* argv[]) : m_argc(argc), m_argv(argv) {}

static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\n\r");
    return s.substr(a, b - a + 1);
}

static void split_comma(const std::string& s, std::vector<std::string>& out) {
    std::istringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = trim(token);
        if (!token.empty()) out.push_back(token);
    }
}

Config CLI::parse() {
    Config cfg;

    for (int i = 1; i < m_argc; i++) {
        std::string arg = m_argv[i];

        if (arg == "-h" || arg == "--help") {
            cfg.showHelp = true;
        }
        else if (arg == "-v" || arg == "--version") {
            cfg.showVersion = true;
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < m_argc) {
                cfg.c_outputFile = m_argv[++i];
            }
            else {
                std::cerr << "Error: missing file name after " << arg << "\n";
            }
        }
        else if (arg == "-i" || arg == "--include") {
            if (i + 1 < m_argc) {
                std::string patterns = m_argv[++i];
                split_comma(patterns, cfg.c_includePatterns);
            }
            else {
                std::cerr << "Error: missing pattern after " << arg << "\n";
            }
        }
        else {
            cfg.c_paths.push_back(arg);
        }
    }
    if (cfg.c_paths.empty()) {
        cfg.c_paths.push_back(".");
    }
    return cfg;
}

void CLI::print_help() {
    std::cout << "Usage: " << TOOL_NAME << " [paths...] [options]\n\n"
        << "Package a repository (or files) into an LLM-friendly text file.\n\n"
        << "Options:\n"
        << "  -h, --help            Display this help message\n"
        << "  -v, --version         Display current version information\n"
        << "  -o, --output <file>   Write packaged output to file (default: stdout)\n"
        << "  -i, --include <globs> Comma-separated glob(s) to include, e.g. \"*.cpp,*.h\"\n\n"
        << "Examples:\n"
        << "  " << TOOL_NAME << " .\n"
        << "  " << TOOL_NAME << " /path/to/repo\n"
        << "  " << TOOL_NAME << " src/main.cpp src/util.cpp -o context.txt\n"
        << "  " << TOOL_NAME << " . --include \"*.cpp,*.h,*.md\"\n";
}
