#include "Compressor.h"
#include <sstream>
#include <unordered_set>
#include <cctype>

using namespace rcpack;

std::vector<std::string> Compressor::extractChunks(const std::string &s, const std::string &ext) {
    std::vector<std::string> chunks;
    std::unordered_set<std::string> seen;

    auto lines = split_lines(s);
    std::vector<size_t> offsets(lines.size() + 1, 0);
    for (size_t i = 0; i < lines.size(); ++i) {
        offsets[i+1] = offsets[i] + lines[i].size() + 1;
    }

    auto is_c_like_sig = [&](const std::string &trimmed) -> bool {
        if (trimmed.empty()) return false;
        if (starts_with(trimmed, "class ") || starts_with(trimmed, "struct ")) return true;
        if (starts_with(trimmed, "function ") || trimmed.find(" function ") != std::string::npos) return true;
        
        if (trimmed.find('(') != std::string::npos && trimmed.find(')') != std::string::npos) {
            if (trimmed.find('{') != std::string::npos) return true;
            if (!trimmed.empty() && trimmed.back() == '{') return true;
        }
        
        if (trimmed.find("=>") != std::string::npos && trimmed.find('{') != std::string::npos) return true;
        return false;
    };

    auto is_python_sig = [&](const std::string &trimmed) -> bool {
        if (starts_with(trimmed, "def ") || starts_with(trimmed, "class ")) return true;
        return false;
    };

    for (size_t i = 0; i < lines.size(); ++i) {
        std::string trimmed = trim(lines[i]);
        bool matched = false;
        if (ext == "py") {
            if (is_python_sig(trimmed)) matched = true;
        } else {
            if (is_c_like_sig(trimmed)) matched = true;
        }
        if (!matched) continue;

        size_t pos = offsets[i];
        std::string preComments = collect_preceding_comment_block_lines(lines, i);

        // size_t bracePosByte = std::string::npos;
        // size_t braceLineIdx = std::string::npos;
      
        size_t scanFrom = pos;
        size_t foundBraceByte = std::string::npos;
        for (size_t b = scanFrom; b < s.size(); ++b) {
            if (s[b] == '{') { foundBraceByte = b; break; }
            if (ext == "py" && s[b] == '\n') break;
        }
        std::string innerComments;
        if (foundBraceByte != std::string::npos) {
            size_t braceEnd = find_matching_brace_safe(s, foundBraceByte);
            if (braceEnd != std::string::npos && braceEnd > foundBraceByte) {
                innerComments = collect_comments_in_range(s, foundBraceByte, braceEnd + 1);
            }
        }
        
            std::string signature = trimmed;
            size_t p = signature.find('(');
            if (p != std::string::npos) {
                size_t q = signature.find(')', p);
                if (q != std::string::npos) {
                    signature = trim(signature.substr(0, q + 1));
                } else {
                    size_t bracePos = signature.find('{');
                    if (bracePos != std::string::npos) signature = trim(signature.substr(0, bracePos));
                }
            } else {
                if (!signature.empty() && signature.back() == '{') {
                    signature = trim(signature.substr(0, signature.size() - 1));
                }
            }

            if (ext == "py" && !signature.empty() && signature.back() != ':') signature += ":";

            auto limit_comment_lines = [](const std::string &comments, size_t maxLines = 6) -> std::string {
                if (comments.empty()) return "";
                std::istringstream iss(comments);
                std::string line;
                std::vector<std::string> keep;
                while (std::getline(iss, line) && keep.size() < maxLines) {
                    std::string t = trim(line);
                    if (!t.empty()) keep.push_back(t);
                }
                return join(keep, "\n");
            };

            std::string innerCommentsLimited = limit_comment_lines(innerComments, 6);

            std::string snippet;
            if (!preComments.empty()) {
                snippet += preComments;
                if (snippet.back() != '\n') snippet += '\n';
            }
            snippet += signature;

            if (!innerCommentsLimited.empty()) {
                snippet += " {\n";
                snippet += innerCommentsLimited;
                snippet += "\n  /* ... */\n}";
            } else {
                if (ext == "py") snippet += "  # ...";
                else snippet += " { /* ... */ }";
            }

            snippet = trim(snippet);
            if (snippet.empty()) continue;
            if (seen.insert(snippet).second) chunks.push_back(snippet);

    }

    if (chunks.empty()) {
        std::vector<std::string> topComments;
        for (size_t i = 0; i < lines.size() && i < 200; ++i) {
            std::string t = trim(lines[i]);
            if (t.empty()) continue;
            if (starts_with(t, "//")) {
                topComments.push_back(t);
            } else if (t.size() >= 2 && starts_with(t, "/*")) {
                
                std::ostringstream oss;
                oss << t;
                size_t j = i + 1;
                while (j < lines.size()) {
                    oss << '\n' << trim(lines[j]);
                    if (lines[j].find("*/") != std::string::npos) { i = j; break; }
                    ++j;
                }
                topComments.push_back(oss.str());
            } else {
                break;
            }
        }
        if (!topComments.empty()) {
            std::string top = join(topComments, "\n");
            chunks.push_back(top);
        }
    }

    return chunks;
}

std::string Compressor::process(const std::string &content,
                                      const std::string &path,
                                      bool compress,
                                      bool removeComments,
                                      bool removeEmptyLines) {
    std::string data = content;
    if (removeComments) data = stripComments(data);
    if (removeEmptyLines) {
        std::istringstream iss(data);
        std::ostringstream oss;
        std::string line;
        while (std::getline(iss, line)) {
            bool all_ws = true;
            for (char c : line) if (!std::isspace((unsigned char)c)) { all_ws = false; break; }
            if (!all_ws) oss << line << '\n';
        }
        data = oss.str();
    }
    if (!compress) return data;

    auto chunks = extractChunks(data, extension(path));
    if (chunks.empty()) {
        if (!data.empty() && data.back() != '\n') data.push_back('\n');
        return data;
    }

    std::ostringstream out;
    for (size_t i = 0; i < chunks.size(); ++i) {
        if (i) out << "\n⋮----\n";
        out << chunks[i];
    }
    std::string result = out.str();
    if (!result.empty() && result.back() != '\n') result.push_back('\n');
    return result;
}

std::string rcpack::Compressor::extension(const std::string &path) {
    auto pos = path.find_last_of('.');
    if (pos == std::string::npos) return "";
    std::string ext = path.substr(pos + 1);
    return toLower(ext);
}

std::string rcpack::Compressor::stripComments(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    bool inBlock = false, inLine = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!inBlock && !inLine && i + 1 < s.size() && s[i] == '/' && s[i + 1] == '/') {
            inLine = true; ++i; continue;
        }
        if (!inBlock && !inLine && i + 1 < s.size() && s[i] == '/' && s[i + 1] == '*') {
            inBlock = true; ++i; continue;
        }
        if (inLine && s[i] == '\n') { inLine = false; out.push_back('\n'); continue; }
        if (inBlock && i + 1 < s.size() && s[i] == '*' && s[i + 1] == '/') { inBlock = false; ++i; continue; }
        if (!inBlock && !inLine) out.push_back(s[i]);
    }
    return out;
}
