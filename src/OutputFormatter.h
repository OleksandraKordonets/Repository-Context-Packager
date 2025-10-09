#pragma once

#include <ostream>
#include <vector>
#include <filesystem>
#include "RepositoryScanner.h"
#include "FileReader.h"
#include "GitInfoCollector.h"
#include "Config.h"

namespace rcpack {

    class OutputFormatter {
        std::ostream &out_;
        void printTree(const std::vector<FileEntry>& files, const std::filesystem::path &root);
    public:
        OutputFormatter(std::ostream &out);
        void generate(const std::filesystem::path &root,
              const Config &cfg,
              const GitInfo &git,
              const ScanResult &scan,
              const std::vector<FileContent> &contents);
    };
}