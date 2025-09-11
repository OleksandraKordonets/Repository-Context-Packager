#include <iostream>
#include <string>
#include "cli.h"
#include "Config.h"

int main(int argc, char* argv[]) {
    CLI cli(argc, argv);
    Config cfg = cli.parse();

    if (cfg.showHelp) {
        cli.print_help();
        return 0;
    }
    if (cfg.showVersion) {
        std::cout << TOOL_NAME << ": v" << TOOL_VERSION << std::endl;
        return 0;
    }
    
    for (const auto& path : cfg.c_paths) {
        std::cout << "# Repository Context\n\n";

        std::cout << "## File System Location\n";
        std::cout << path << "\n\n";

        std::cout << "## Git Info\n";
        std::cout << "- Commit: (not implemented yet)\n";
        std::cout << "- Branch: (not implemented yet)\n";
        std::cout << "- Author: (not implemented yet)\n";
        std::cout << "- Date: (not implemented yet)\n\n";

        std::cout << "## Structure\n";
        std::cout << "(structure for " << path << " not implemented yet)\n\n";

        std::cout << "## File Contents\n\n";
        std::cout << "### File: ";
        std::cout << "(file contents for " << path << " not implemented yet)\n\n";

        std::cout << "## Summary\n";
        std::cout << "- Total files: (not implemented yet)\n";
        std::cout << "- Total lines: (not implemented yet)\n\n";
    }
    return 0;
}