#pragma once

#include <string>
#include <filesystem>

namespace rcpack {

    struct FileContent {
        std::string content;
        bool truncated = false;
        size_t lines = 0;
    };

    class FileReader {
        size_t fr_maxBytes;
    public:
        // maxBytes default 16KB if file > maxBytes we'll read only first maxBytes and set truncated
        explicit FileReader(size_t maxBytes = 16 * 1024);
        FileContent readFile(const std::filesystem::path &p) const;
    };
}