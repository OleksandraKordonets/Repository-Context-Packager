# Repository-Context-Packager
OSD600 - Release 0.1

This is a command-line tool that analyzes local git repositories and creates a text file containing repository content optimized for sharing with Large Language Models (LLMs).

When developers want to get help from ChatGPT or other LLMs about their code, they often struggle with how to share their codebase effectively. They might copy-paste individual files, but this loses important context about the project structure, dependencies, and relationships between files. This tool will solve this by automatically collecting and formatting repository content into a single, well-structured text file that can be easily shared with any LLM.

---

## Installation:

### Prerequisites
- **g++** (C++17 or later)
- **Git** (to clone the repository)

### Clone the repository:
```
git clone https://github.com/OleksandraKordonets/Repository-Context-Packager.git
cd Repository-Context-Packager
```

### Build
From inside the repo folder, run:
```
g++ src/main.cpp src/cli.cpp src/RepositoryScanner.cpp src/FileReader.cpp src/GitInfoCollector.cpp src/OutputFormatter.cpp -o repository-context-packager.exe
```
**Note:** Using `*.cpp` instead of listing all the files may not work reliably across platforms, so it’s recommended to use the full command above.

### Run
Once built, you can run the tool from the command line:
```
./repository-context-packager --help
```

### Examples
```
# Package the current directory
./repository-context-packager .

# Package a specific repo directory
./repository-context-packager /home/student1/Seneca/major-project

# Package specific files
./repository-context-packager src/main.js src/utils.h

# Package with output file
./repository-context-packager . -o context.txt

# Package only JavaScript files
./repository-context-packager . --include "*.cpp"
```

---

## Features:

**Command-Line Interface**  
Supports options like --help, --version, --output, and --include to customize what gets scanned.

**Repository Scanning**  
Walks through provided paths (files or directories) and collects all matching source files.

**File Reading with Size Limit**  
Reads file contents safely (up to 16 KB by default) and marks large files as truncated.

**Git Info Collection**  
Captures commit hash, branch, and author from the repository (when available).

**Output Formatting**  
Generates a structured text output that shows file system structure, Git info, and file contents.