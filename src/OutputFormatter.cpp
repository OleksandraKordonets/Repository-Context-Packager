#include "OutputFormatter.h"
#include <system_error>
#include <iostream>
#include <map>
#include <algorithm>

using namespace rcpack;

OutputFormatter::OutputFormatter(std::ostream &out): out_(out){}

void OutputFormatter::printTree(const std::vector<FileEntry>& files, const std::filesystem::path& root) {
    //std::cerr << "OutputFormatter::printTree: received " << files.size() << " files; root = " << root << "\n";

    // Build a basic tree: map of directories to their children
    std::map<std::string, std::vector<std::string>> tree;
    for (const auto& f : files) {
        std::error_code ec;
        std::filesystem::path rel = std::filesystem::relative(f.path, root, ec);
        std::string rels;
        if (ec) {
            // Could not compute relative path: log and fallback to filename only
            std::cerr << "  relative() failed for: " << f.path << " -> " << ec.message() << "\n";
            rels = f.path.filename().generic_string();
            // Put it under "." to ensure it's visible in the output tree
            tree["."].push_back(rels);
            continue;
        }
        else {
            rels = rel.generic_string();
        }

        std::filesystem::path p(rels);
        std::string parent = p.parent_path().generic_string();
        if (parent.empty()) parent = ".";
        tree[parent].push_back(p.filename().generic_string());

        // DEBUG: print what we added
        //std::cerr << "  file: " << f.path << " -> rel: " << rels << " parent: " << parent << "\n";
    }

    // Print tree in deterministic order
    out_ << "```\n";
    std::vector<std::string> keys;
    for (auto& kv : tree) keys.push_back(kv.first);
    std::sort(keys.begin(), keys.end());
    for (auto& k : keys) {
        // sort children for deterministic output
        auto &children = tree[k];
        std::sort(children.begin(), children.end());

        if (k == ".") {
            for (auto& name : children) out_ << name << "\n";
        }
        else {
            out_ << k << "/\n";
            for (auto& name : children) out_ << "  " << name << "\n";
        }
    }
    out_ << "```\n";
}


void OutputFormatter::generate(const std::filesystem::path& root,
    const Config& cfg,
    const GitInfo& git,
    const ScanResult& scan,
    const std::vector<FileContent>& contents) {
    out_ << "# Repository Context\n\n";
    out_ << "## File System Location\n\n";
    out_ << std::filesystem::absolute(root).generic_string() << "\n\n";

    out_ << "## Git Info\n\n";
    if (!git.isRepo) {
        out_ << "Not a git repository\n\n";
    }
    else {
        out_ << "- Commit: " << git.commitSHA << "\n";
        out_ << "- Branch: " << git.branch << "\n";
        out_ << "- Author: " << git.author << "\n";
        out_ << "- Date: " << git.date << "\n\n";
    }

    out_ << "## Structure\n";
    printTree(scan.files, root);
    out_ << "\n";

    if (!cfg.dirsOnly) {
        out_ << "## File Contents\n\n";

        for (size_t i = 0; i < scan.files.size(); ++i) {
            auto& fe = scan.files[i];
            out_ << "### File: ";
            std::error_code ec;
            auto rel = std::filesystem::relative(fe.path, root, ec);
            if (!ec) out_ << rel.generic_string() << "\n";
            else out_ << fe.path.generic_string() << "  (failed to compute relative: " << ec.message() << ")\n";

            out_ << "```\n";
            if (i < contents.size()) {
                out_ << contents[i].content;
                if (contents[i].truncated) out_ << "\n...(truncated)\n";
            }
            else {
                out_ << "(no content read)\n";
            }
            out_ << "```\n\n";
        }

        // Summary
        size_t totalLines = 0;
        for (auto& c : contents) totalLines += c.lines;
        out_ << "## Summary\n";
        out_ << "- Total files: " << scan.files.size() << "\n";
        out_ << "- Total lines: " << totalLines << "\n";

    } else {
        // print a short note
        out_ << "## File Contents\n\n";
        out_ << "(skipped: directory-only mode)\n\n";
    }
}