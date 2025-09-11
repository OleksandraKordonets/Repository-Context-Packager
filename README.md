# Repository-Context-Packager
OSD600 - Release 0.1

# Modules:

## Config

The Config module defines a simple data container class that stores all the settings and options provided by the user when running the tool. It doesn’t do any logic itself. it just holds values so the rest of the program can use them.

### Key parts:
- c_paths: List of file or directory paths the user wants to analyze.
- c_outputFile: Output file name (if provided with -o / --output).
- c_includePatterns: File patterns/extensions to include (if provided with -i / --include).
- showHelp: Flag set to true when help is used.
- showVersion: Flag set to true when version is used.

## CLI

### The CLI module is responsible for:
Reading the arguments the user provides when running the program.
Parsing those arguments and filling in a Config object with the correct values.

### Key parts:
- Constructor: Stores argc and argv from main().
- parse(): Walks through all arguments, sets flags, and records paths/options inside a Config.
- print_help(): Displays available commands and examples.

The CLI module ensures that no matter how the user runs the program, the rest of the code has a consistent Config object to work with.

## Utility

The Utils.h file provides a set of helper functions for common string operations and pattern processing used throughout the project. All functions are implemented as inline for easy inclusion in headers without separate compilation.

### Key Functions:
- trim(const std::string &s) – Removes leading and trailing whitespace from a string.
- split_comma(const std::string &s, std::vector<std::string> &out) – Splits a comma-separated string into individual items and trims each one.
- toLower(std::string s) – Converts all characters in a string to lowercase.
- splitPatterns(const std::string &in) – Converts a comma-separated list of file patterns (like "*.js,*.py") into a vector of individual patterns.

These utilities help simplify processing of CLI input and file patterns, making the rest of the code cleaner and easier to maintain.

## RepositoryScanner

The RepositoryScanner module is responsible for traversing directories and collecting files based on user-specified patterns. It helps your tool analyze repository contents efficiently and selectively.

### Key Features:

#### Pattern Matching:
Accepts include patterns like "*.js", ".js", or "js" and normalizes them internally for matching file extensions. Only files that match the specified patterns are included.

#### File and Directory Traversal:
Can scan both individual files and entire directories, including nested subdirectories recursively.

#### Error Handling:
Skips files or directories it cannot access and logs warnings to stderr without crashing.

#### Result Structure:
Returns a ScanResult object that contains:
- files: List of FileEntry objects with path and size.
- skipped: List of files or directories that were skipped due to errors or nonexistence.