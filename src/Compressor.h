#pragma once
#include "utils.h"
#include <string>
#include <vector>

namespace rcpack {
    class Compressor {
    public:
        Compressor() = default;
        ~Compressor() = default;

        std::string process(const std::string &content,
                            const std::string &path,
                            bool compress,
                            bool removeComments,
                            bool removeEmptyLines);

    private:
        std::string extension(const std::string &path);
        std::string stripComments(const std::string &s);
        std::vector<std::string> extractChunks(const std::string &s, const std::string &ext);
    };
}