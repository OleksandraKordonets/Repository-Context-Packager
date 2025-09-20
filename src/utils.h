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