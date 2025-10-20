#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <filesystem>
#include <chrono> 
#include <iostream>

namespace rcpack {

    inline bool isFileRecent(const std::filesystem::path &p){
        constexpr int MAX_RECENT_DAYS = 7;    
        try{
        //get the last  change time 
        auto lastChangeTime = std::filesystem::last_write_time(p);
        //get current time and calculate the difference    
        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point daysAgo =  now - std::chrono::hours(24 * MAX_RECENT_DAYS); 
        //change file time to  system_clock
        std::chrono::system_clock::time_point file_time = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            lastChangeTime - std::filesystem::file_time_type::clock::now() + now);
        
           return file_time >= daysAgo;
        }catch(const std::exception&e){
            std::cerr<<"Could not check time for "<< p <<": "<< e.what()<<'\n';
            return false;
        }
    }


    inline std::string trim(const std::string &s) {
        size_t a = s.find_first_not_of(" \t\n\r");
        if (a==std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t\n\r");
        return s.substr(a, b-a+1);
    }

    // Unified pattern parser: accepts "a,b,c" or "a, b , c" and returns trimmed parts.
    inline std::vector<std::string> parse_patterns(const std::string &in) {
        std::vector<std::string> out;
        std::string cur;
        for (char c : in) {
            if (c == ',') {
                if (!cur.empty()) out.push_back(trim(cur));
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
        if (!cur.empty()) out.push_back(trim(cur));
        return out;
    }

    // Backwards-compatible wrappers (no API change required in callers)
    inline void split_comma(const std::string& s, std::vector<std::string>& out) {
        auto r = parse_patterns(s);
        out.insert(out.end(), r.begin(), r.end());
    }

    inline std::vector<std::string> splitPatterns(const std::string &in){
        return parse_patterns(in);
    }

    inline std::vector<std::string> split_lines(const std::string &s) {
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

    inline std::string toLower(std::string s){
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
        return s;
    }

    // utils.h additions (place near other helpers)
    inline std::string normalize_extension_token(const std::string &raw) {
        // Returns a normalized extension form ".ext" for tokens like "*.js", ".js", "js".
        // Returns empty string if token doesn't look like a simple extension.
        std::string p = trim(raw);
        if (p.empty()) return {};

        // strip surrounding quotes if any
        if (p.size() >= 2 && ((p.front()=='"' && p.back()=='"') || (p.front()=='\'' && p.back()=='\''))) {
            p = p.substr(1, p.size() - 2);
            p = trim(p);
            if (p.empty()) return {};
        }

        // turn "*.ext" -> ".ext"
        if (p.size() >= 2 && p[0] == '*' && p[1] == '.') p = p.substr(1);

        // if starts with '.' like ".js"
        if (!p.empty() && p[0] == '.') return toLower(p);

        // if token is alphanumeric only (e.g., "js", "cpp") treat as extension
        bool onlyAlnum = true;
        for (unsigned char c : p) {
            if (!std::isalnum(c)) { onlyAlnum = false; break; }
        }
        if (onlyAlnum) return std::string(".") + toLower(p);

        // otherwise not an extension-style token
        return {};
    }

    // for future feature on removing whitespace lines
    inline bool isWhitespaceLine(const std::string &line) {
        for (char c : line) {
            if (!std::isspace(static_cast<unsigned char>(c))) return false;
        }
        return true;
    }

    // join lines with a separator
    inline std::string join(const std::vector<std::string> &lines, const std::string &sep = "\n") {
        std::ostringstream oss;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i) oss << sep;
            oss << lines[i];
        }
        return oss.str();
    }

    inline bool starts_with(const std::string &s, const std::string &pref) {
        return s.size() >= pref.size() && s.compare(0, pref.size(), pref) == 0;
    }

    inline size_t find_outside_quotes(const std::string &line, const std::string &pat) {
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

    inline bool line_is_comment_only(const std::string &line, bool debug = false) {
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

    inline size_t find_matching_brace_safe(const std::string &s, size_t openPos) {
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

    inline std::string collect_comments_in_range(const std::string &s, size_t start, size_t end) {
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

    inline std::string collect_preceding_comment_block_lines(const std::vector<std::string> &lines, size_t lineIdx) {
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
        return join(collected, "\n");
    }

}