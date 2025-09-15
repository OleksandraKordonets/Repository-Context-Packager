#pragma once

#include <string>
#include <optional>

namespace rcpack {

    struct GitInfo {
        bool isRepo = false;
        std::string commitSHA;
        std::string branch;
        std::string author;
        std::string date;
    };

    class GitInfoCollector {
        std::string git_repoPath;
        std::string runCmd(const std::string &cmd) const;
    public:
        explicit GitInfoCollector(const std::string &path);
        GitInfo collect();
    };
}