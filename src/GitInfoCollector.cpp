#include "GitInfoCollector.h"
#include <cstdio>
#include <memory>
#include <array>
#include <iostream>
#include <filesystem>
#include <system_error>

using namespace rcpack;
namespace fs = std::filesystem;

#if defined(_WIN32)
  #define POPEN _popen
  #define PCLOSE _pclose
  static constexpr const char* NULLDEV = "NUL";
#else
  #define POPEN popen
  #define PCLOSE pclose
  static constexpr const char* NULLDEV = "/dev/null";
#endif

// RAII helper to temporarily change cwd and restore it on destruction
class ScopedChdir {
public:
    explicit ScopedChdir(const fs::path &newCwd) {
        try {
            oldCwd_ = fs::current_path();
            fs::current_path(newCwd);
            active_ = true;
        } catch (...) {
            active_ = false;
        }
    }
    ~ScopedChdir() noexcept {
        if (active_) {
            try { fs::current_path(oldCwd_); } catch(...) {}
        }
    }
private:
    fs::path oldCwd_;
    bool active_ = false;
};

GitInfoCollector::GitInfoCollector(const std::string &path): git_repoPath(path) {}

static std::string trimNewline(std::string s) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}

std::string GitInfoCollector::runCmd(const std::string &cmd) const {
    std::array<char, 256> buffer;
    std::string result;
    FILE* pipe = nullptr;
    // POPEN is defined to _popen on Windows and popen on POSIX
    pipe = POPEN(cmd.c_str(), "r");
    if (!pipe) return {};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    PCLOSE(pipe);
    return trimNewline(result);
}

GitInfo GitInfoCollector::collect() {
    GitInfo g;
    try {
        fs::path start = fs::absolute(git_repoPath);
        // If user passed a file, start from its parent
        if (fs::exists(start) && fs::is_regular_file(start)) start = start.parent_path();

        // Walk upwards to find .git folder
        fs::path repoRoot;
        for (fs::path p = start; ; p = p.parent_path()) {
            if (p.empty()) break;
            if (fs::exists(p / ".git")) { repoRoot = p; break; }
            if (p == p.root_path()) break;
        }

        if (repoRoot.empty()) {
            // Not a git repo
            g.isRepo = false;
            return g;
        }

        g.isRepo = true;

        // Use ScopedChdir so commands are simple (avoid embedding paths into the command)
        ScopedChdir sc(repoRoot);

        // Get commit SHA
        g.commitSHA = runCmd(std::string("git rev-parse HEAD 2> ") + NULLDEV);
        // Branch
        g.branch = runCmd(std::string("git rev-parse --abbrev-ref HEAD 2> ") + NULLDEV);
        // Author and date (separated by |)
        std::string logres = runCmd(std::string("git log -1 --pretty=format:\"%an <%ae>|%ad\" 2> ") + NULLDEV);
        if (!logres.empty()) {
            auto pos = logres.find('|');
            if (pos != std::string::npos) {
                g.author = logres.substr(0, pos);
                g.date = logres.substr(pos + 1);
            } else {
                g.author = logres;
            }
        }
    } catch (const std::exception &ex) {
        std::cerr << "GitInfoCollector error: " << ex.what() << "\n";
    }
    return g;
}
