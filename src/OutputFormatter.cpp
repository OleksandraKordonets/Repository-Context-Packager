#include "OutputFormatter.h"

#include <iostream>
#include <map>
#include <algorithm>

using namespace rcpack;

OutputFormatter::OutputFormatter(std::ostream &out): out_(out){}

void OutputFormatter::printTree(const std::vector<FileEntry>& files, const std::filesystem::path &root){
    // Build a basic tree: map of directories to their children
    std::map<std::string, std::vector<std::string>> tree;

    for (const auto &f : files){
        auto rel = std::filesystem::relative(f.path, root);
        std::string rels = rel.generic_string();
        std::filesystem::path p(rels);
        std::string parent = p.parent_path().generic_string();
        if (parent.empty()) parent = ".";
        tree[parent].push_back(p.filename().generic_string());
    }
    // Print top-level entries (simple, not perfect tree printer)
    out_ << "```\n";

    // sort keys for deterministic output
    std::vector<std::string> keys;
    for (auto &kv : tree) keys.push_back(kv.first);

    std::sort(keys.begin(), keys.end());
    for (auto &k : keys){
        if (k=="."){
            
        } else {
            out_ << k << "/\n";
            for (auto &name : tree[k]) out_ << " " << name << "\n";
        }
    }
    out_ << "``\n";
}

void OutputFormatter::generate(const std::filesystem::path &root,
const GitInfo &git,
const ScanResult &scan,
const std::vector<FileContent> &contents){
    out_ << "# Repository Context\n\n";
    out_ << "## File System Location\n\n";
    out_ << std::filesystem::absolute(root).generic_string() << "\n\n";

    out_ << "## Git Info\n\n";
    if (!git.isRepo){
        
    } else {
        out_ << "- Commit: " << git.commitSHA << "\n";
        out_ << "- Branch: " << git.branch << "\n";
        out_ << "- Author: " << git.author << "\n";
        out_ << "- Date: " << git.date << "\n\n";
    }

    out_ << "## Structure\n";
    printTree(scan.files, root);

    out_ << "\n## File Contents\n\n";
    for (size_t i=0;i<scan.files.size();++i){
        auto &fe = scan.files[i];
        out_ << "### File: " << std::filesystem::relative(fe.path, root).generic_string() << "\n";
        out_ << "```\n";
        out_ << contents[i].content;
        if (contents[i].truncated) out_ << "\n...(truncated)\n";
        out_ << "```\n\n";
    }

    // Summary
    size_t totalLines = 0;
    for (auto &c : contents) totalLines += c.lines;
    out_ << "## Summary\n";
    out_ << "- Total files: " << scan.files.size() << "\n";
    out_ << "- Total lines: " << totalLines << "\n";
}