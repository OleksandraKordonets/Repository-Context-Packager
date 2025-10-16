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

}