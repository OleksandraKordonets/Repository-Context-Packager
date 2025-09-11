#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

namespace rcpack {

    inline std::string trim(const std::string &s) {
        size_t a = s.find_first_not_of(" \t\n\r");
        if (a==std::string::npos) return "";
        size_t b = s.find_last_not_of(" \t\n\r");
        return s.substr(a, b-a+1);
    }

    inline void split_comma(const std::string& s, std::vector<std::string>& out) {
        std::istringstream ss(s);
        std::string token;
        while (std::getline(ss, token, ',')) {
            token = trim(token);
            if (!token.empty()) out.push_back(token);
        }
    }

    inline std::string toLower(std::string s){
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
        return s;
    }

    // Convert --include patterns like "*.js,*.py" -> vector of patterns
    inline std::vector<std::string> splitPatterns(const std::string &in){
        std::vector<std::string> out;
        std::string cur;
        for(char c: in){
            if (c==','){
                if(!cur.empty()) out.push_back(trim(cur));
                cur.clear();
            } else cur.push_back(c);
        }
        if(!cur.empty()) out.push_back(trim(cur));
        return out;
    }
}