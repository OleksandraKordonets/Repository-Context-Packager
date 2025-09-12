#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "FileReader.h"

using namespace rcpack;

FileReader::FileReader(size_t maxBytes): fr_maxBytes(maxBytes){}

FileContent FileReader::readFile(const std::filesystem::path &p) const{
    FileContent out;
    std::ifstream in(p, std::ios::in | std::ios::binary);

    if (!in){
        std::cerr << "Error: cannot open file " << p << " for reading\n";
        return out;
    }

    // check file size if possible
    std::error_code ec;
    auto fsize = std::filesystem::file_size(p, ec);
    bool useTruncate = false;
    if (!ec && fsize > (uintmax_t)fr_maxBytes) useTruncate = true;

    //case 1 - Truncated read
    if (useTruncate){
        std::string buffer;
        buffer.reserve(fr_maxBytes + 1);
        char *tmp = new char[fr_maxBytes];
        in.read(tmp, (std::streamsize)fr_maxBytes);
        std::streamsize read = in.gcount();
        buffer.assign(tmp, (size_t)read);
        delete[] tmp;
        out.content = buffer;
        out.truncated = true;
        // count lines in buffer
        out.lines = std::count(out.content.begin(), out.content.end(), '\n');

    } else { //case 2 - Full read
        // read whole file line by line 
        std::string line;
        std::ostringstream oss;
        size_t lines = 0;
        
        while (std::getline(in, line)){
            oss << line << '\n';
            lines++;
        }
        out.content = oss.str();
        out.lines = lines;
    }
    return out;
}