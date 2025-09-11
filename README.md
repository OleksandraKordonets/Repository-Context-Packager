# Repository-Context-Packager
OSD600 - Release 0.1

# Modules:

## Config

The Config module defines a simple data container class that stores all the settings and options provided by the user when running the tool. It doesn’t do any logic itself. it just holds values so the rest of the program can use them.

Key parts:
c_paths: List of file or directory paths the user wants to analyze.
c_outputFile: Output file name (if provided with -o / --output).
c_includePatterns: File patterns/extensions to include (if provided with -i / --include).
showHelp: Flag set to true when help is used.
showVersion: Flag set to true when version is used.

## CLI

The CLI module is responsible for:
Reading the arguments the user provides when running the program.
Parsing those arguments and filling in a Config object with the correct values.

Key parts:
Constructor: Stores argc and argv from main().
parse(): Walks through all arguments, sets flags, and records paths/options inside a Config.
print_help(): Displays available commands and examples.

The CLI module ensures that no matter how the user runs the program, the rest of the code has a consistent Config object to work with.