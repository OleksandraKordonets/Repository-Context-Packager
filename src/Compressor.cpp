#include "Compressor.h"
#include <sstream>
#include <unordered_set>
#include <cctype>

using namespace rcpack;

static bool starts_with(const std::string &s, const std::string &pref) {
    return s.size() >= pref.size() && s.compare(0, pref.size(), pref) == 0;
}

static size_t find_outside_quotes(const std::string &line, const std::string &pat) {
    if (pat.empty()) return std::string::npos;
    bool in_single = false, in_double = false, in_backtick = false;
    for (size_t i = 0; i + pat.size() <= line.size(); ++i) {
        char c = line[i];
        if (!in_single && !in_double && !in_backtick && c == '"') { in_double = true; continue; }
        if (in_double) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') { in_double = false; continue; }
            continue;
        }
        if (!in_single && !in_double && !in_backtick && c == '\'') { in_single = true; continue; }
        if (in_single) {
            if (c == '\\') { ++i; continue; }
            if (c == '\'') { in_single = false; continue; }
            continue;
        }
        if (!in_single && !in_double && !in_backtick && c == '`') { in_backtick = true; continue; }
        if (in_backtick) {
            if (c == '\\') { ++i; continue; }
            if (c == '`') { in_backtick = false; continue; }
            continue;
        }

        if (!in_single && !in_double && !in_backtick) {
            if (line.compare(i, pat.size(), pat) == 0) return i;
        }
    }
    return std::string::npos;
}

static std::string join_lines(const std::vector<std::string> &lines, const std::string &sep = "\n") {
    std::ostringstream oss;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i) oss << sep;
        oss << lines[i];
    }
    return oss.str();
}

static bool line_is_comment_only(const std::string &line, bool debug = false) {
    size_t i = 0;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
    if (i >= line.size()) return false;

    char c = line[i];

    if (std::isalpha(static_cast<unsigned char>(c))) return false;

    if (line[i] == '#') return true;
    if (i + 1 < line.size() && line[i] == '/' && line[i+1] == '/') return true;  // //
    if (i + 1 < line.size() && line[i] == '/' && line[i+1] == '*') return true;   // /*
    if (i + 1 < line.size() && line[i] == '*' && line[i+1] == '/') return true;   // */

    if (debug) {
        if (line.find('(') != std::string::npos || line.find("==") != std::string::npos) {
            std::cerr << "[compressor-debug] line_is_comment_only true for suspicious line: \"" 
                      << line << "\"\n";
        }
    }
    return false;
}

static size_t find_matching_brace_safe(const std::string &s, size_t openPos) {
    if (openPos >= s.size() || s[openPos] != '{') return std::string::npos;
    int depth = 0;
    bool in_single = false, in_double = false, in_backtick = false;
    for (size_t i = openPos; i < s.size(); ++i) {
        char c = s[i];
        if (!in_single && !in_double && !in_backtick && c == '/' && i + 1 < s.size() && s[i+1] == '/') {
            i += 2;
            while (i < s.size() && s[i] != '\n') ++i;
            continue;
        }
        if (!in_single && !in_double && !in_backtick && c == '/' && i + 1 < s.size() && s[i+1] == '*') {
            i += 2;
            while (i + 1 < s.size() && !(s[i] == '*' && s[i+1] == '/')) ++i;
            if (i + 1 < s.size()) i += 1;
            continue;
        }
        if (!in_single && !in_double && !in_backtick && c == '\'') { in_single = true; continue; }
        if (in_single) {
            if (c == '\\') { ++i; continue; }
            if (c == '\'') { in_single = false; continue; }
            continue;
        }
        if (!in_single && !in_double && !in_backtick && c == '"') { in_double = true; continue; }
        if (in_double) {
            if (c == '\\') { ++i; continue; }
            if (c == '"') { in_double = false; continue; }
            continue;
        }
        if (!in_single && !in_double && !in_backtick && c == '`') { in_backtick = true; continue; }
        if (in_backtick) {
            if (c == '\\') { ++i; continue; }
            if (c == '`') { in_backtick = false; continue; }
            continue;
        }

        if (c == '{') ++depth;
        else if (c == '}') {
            --depth;
            if (depth == 0) return i;
        }
    }
    return std::string::npos;
}

static std::string collect_comments_in_range(const std::string &s, size_t start, size_t end) {
    std::vector<std::string> comments;
    bool in_single = false, in_double = false, in_backtick = false;
    size_t i = start;
    while (i < end) {
        char c = s[i];

        if (in_single) {
            if (c == '\\') { i += 2; continue; }
            if (c == '\'') { in_single = false; ++i; continue; }
            ++i; continue;
        }
        if (in_double) {
            if (c == '\\') { i += 2; continue; }
            if (c == '"') { in_double = false; ++i; continue; }
            ++i; continue;
        }
        if (in_backtick) {
            if (c == '\\') { i += 2; continue; }
            if (c == '`') { in_backtick = false; ++i; continue; }
            ++i; continue;
        }
        
        if (c == '/' && i + 1 < end && s[i+1] == '/') {
            size_t j = i + 2;
            while (j < end && s[j] != '\n') ++j;
            comments.push_back(trim(s.substr(i, j - i)));
            i = j;
            continue;
        }
        if (c == '/' && i + 1 < end && s[i+1] == '*') {
            size_t j = i + 2;
            while (j + 1 < end && !(s[j] == '*' && s[j+1] == '/')) ++j;
            if (j + 1 < end) j += 2;
            comments.push_back(trim(s.substr(i, std::min(j, end) - i)));
            i = j;
            continue;
        }
        
        if (c == '\'') { in_single = true; ++i; continue; }
        if (c == '"') { in_double = true; ++i; continue; }
        if (c == '`') { in_backtick = true; ++i; continue; }

        ++i;
    }
    if (comments.empty()) return "";
    std::ostringstream oss;
    for (size_t k = 0; k < comments.size(); ++k) {
        if (k) oss << '\n';
        oss << comments[k];
    }
    return oss.str();
}

static std::string collect_preceding_comment_block_lines(const std::vector<std::string> &lines, size_t lineIdx) {
    if (lineIdx == 0) return "";
    ssize_t i = static_cast<ssize_t>(lineIdx) - 1;
    std::vector<std::string> collected;
    int blankAllowed = 1;

    while (i >= 0) {
        std::string trimmed = trim(lines[i]);
        if (trimmed.empty()) {
            if (blankAllowed) {
                blankAllowed--;
                --i;
                continue;
            } else break;
        }
        
        if (line_is_comment_only(trimmed)) {
            collected.push_back(trimmed);
            --i;
            continue;
        }
        
        bool isBlockEnd = false;
        if (trimmed.size() >= 2 && trimmed.substr(0, 2) == "*/") isBlockEnd = true;

        if (isBlockEnd) {
            std::string block;
            ssize_t j = i;
            bool foundStart = false;
            while (j >= 0) {
                size_t pos = find_outside_quotes(lines[j], "/*");
                if (pos != std::string::npos) {
                    bool only_ws_before = true;
                    for (size_t k = 0; k < pos; ++k) {
                        if (!std::isspace(static_cast<unsigned char>(lines[j][k]))) { only_ws_before = false; break; }
                    }
                    if (only_ws_before) {
                        std::ostringstream oss;
                        for (ssize_t k = j; k <= i; ++k) {
                            if (k > j) oss << '\n';
                            oss << trim(lines[k]);
                        }
                        block = oss.str();
                        foundStart = true;
                        i = j - 1;
                        break;
                    }
                }
                --j;
            }
            if (foundStart) {
                collected.push_back(block);
                continue;
            } else {
                break;
            }
        }
        break;
    }

    if (collected.empty()) return "";

    std::reverse(collected.begin(), collected.end());
    return join_lines(collected, "\n");
}

static std::vector<std::string> split_lines(const std::string &s) {
    std::vector<std::string> lines;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == '\n') {
            lines.push_back(s.substr(start, i - start));
            start = i + 1;
        }
    }
    return lines;
}

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

        size_t bracePosByte = std::string::npos;
        size_t braceLineIdx = std::string::npos;
      
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
                return join_lines(keep, "\n");
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
            std::string top = join_lines(topComments, "\n");
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
