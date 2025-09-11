#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>
#include "config.h"

const std::string TOOL_NAME = "repo-context-packager";
const std::string TOOL_VERSION = "0.1";

class CLI {
	int m_argc{};
	char** m_argv{};	
public:	
	CLI(int argc, char* argv[]);
	Config parse();
	void print_help();
};
#endif