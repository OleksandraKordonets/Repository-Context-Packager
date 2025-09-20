#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>

#include "CLI.h"
#include "RepositoryScanner.h"
#include "FileReader.h"
#include "GitInfoCollector.h"
#include "OutputFormatter.h"
#include "utils.h"

using namespace rcpack;
namespace fs = std::filesystem;

// Normalize a user-provided path into an absolute, lexically-normalized path.
// Returns empty path on error.
static fs::path normalizePath(const std::string &raw) {
    try {
        fs::path p = fs::u8path(raw);// accept either slashes
        if (p.is_relative()) p = fs::absolute(p);
        return p.lexically_normal();
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Invalid path: " << raw << " (" << e.what() << ")\n";
        return {};
    }
}

// Walk upward from 'start' to find repository root (folder that contains ".git")
// Returns empty path if not found.
static fs::path findRepoRoot(fs::path start) {
    if (start.empty()) return {};
    start = fs::absolute(start);
    // If start is a file, start from its parent
    if (fs::exists(start) && fs::is_regular_file(start)) start = start.parent_path();
    // Climb up until root
    for (fs::path p = start; !p.empty() && p != p.root_path(); p = p.parent_path()) {
        if (fs::exists(p / ".git")) return p;
    }
    // final check for the root path itself
    if (fs::exists(start.root_path() / ".git")) return start.root_path();
    return {};
}

int main(int argc, char** argv) {
    // parse CLI
    CLI cli(argc, argv);
    Config cfg = cli.parse();

    if (cfg.showHelp) {
        cli.print_help();
        return 0;
    }
    if (cfg.showVersion) {
        std::cout << "repo-context-packager 0.1.0\n";
        return 0;
    }

    if (cfg.c_paths.empty()) {
        std::cerr << "Error: no paths provided. Use -h for help.\n";
        return 1;
    }

    // Normalize and validate input paths
    std::vector<fs::path> normalized;
    for (const auto &raw : cfg.c_paths) {
        fs::path p = normalizePath(raw);
        if (p.empty()) continue;
        if (!fs::exists(p)) {
            std::cerr << "Warning: path does not exist: \"" << raw << "\" -> \"" << p.string() << "\"\n";
            continue;
        }
        normalized.push_back(p);
    }

    if (normalized.empty()) {
        std::cerr << "Error: no valid paths were provided after normalization.\n";
        return 1;
    }

    // Use the first normalized input as the "outputRoot" (the folder whose structure we display)
    fs::path first = normalized.front();
    fs::path outputRoot = fs::is_directory(first) ? first : first.parent_path();

    // Find repo root by walking upwards from the outputRoot
    fs::path repoRoot = findRepoRoot(outputRoot);
    if (repoRoot.empty()) {
        std::cerr << "Info: .git not found from \"" << outputRoot.string() << "\" upwards. Git info will be omitted.\n";
    } else {
        std::cerr << "Info: Git repo root detected at: " << repoRoot.string() << "\n";
    }

    // Convert normalized paths back to strings for the existing scanner API
    std::vector<std::string> scanInputs;
    scanInputs.reserve(normalized.size());
    for (auto &p : normalized) scanInputs.push_back(p.string());

    // Scanner (filter patterns are passed as previously)
    RepositoryScanner scanner(cfg.c_includePatterns);
    auto scanResult = scanner.scanPaths(scanInputs);

     if(cfg.showRecent){
        
        //using algorithm to avoid the manual loops
         auto end = std::remove_if(scanResult.files.begin(), scanResult.files.end(),
        [](const FileEntry& file) { 
            //checking the File via function
            return !isFileRecent(file.path); 
        });
        
        scanResult.files.erase(end, scanResult.files.end());
    }
    // Read files
    FileReader reader(16 * 1024);
    std::vector<FileContent> contents;
    contents.reserve(scanResult.files.size());
    for (auto &fe : scanResult.files) {
        contents.push_back(reader.readFile(fe.path));
    }

    // Git info: use repoRoot if found; otherwise pass outputRoot (collector should handle non-repo case)
    fs::path gitProbePath = repoRoot.empty() ? outputRoot : repoRoot;
    GitInfoCollector gitCollector(gitProbePath.string());
    auto git = gitCollector.collect();

    // Output: determine output stream (stdout or file) and use outputRoot for relative printing
    if (!cfg.c_outputFile.empty()) {
        // normalize output file path too
        fs::path outPath = normalizePath(cfg.c_outputFile);
        // If user provided relative name, normalizePath returns absolute; we can open outPath directly
        std::ofstream ofs(outPath.string());
        if (!ofs) {
            std::cerr << "Error: cannot open output file: " << outPath.string() << "\n";
            return 1;
        }
        OutputFormatter fmt(ofs);
        fmt.generate(outputRoot, git, scanResult, contents);
        ofs.close();
    } else {
        OutputFormatter fmt(std::cout);
        fmt.generate(outputRoot, git, scanResult, contents);
    }

    return 0;
}
