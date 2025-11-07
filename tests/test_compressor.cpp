#include "catch.hpp"

#include "../src/Compressor.h"
#include <string>

using namespace rcpack;

TEST_CASE("Compressor::process removes // comments when removeComments=true", "[Compressor][stripComments]") {
    Compressor c;
    std::string content = "std::string s = \"// not a comment\"; // real comment\nint x = 1;\n";
    std::string out = c.process(content, "file.cpp", false, true, false);

    REQUIRE(out.find("// real comment") == std::string::npos);

    REQUIRE(out.find("int x = 1;") != std::string::npos);
}

TEST_CASE("Compressor::process with compress=true extracts signature snippet", "[Compressor][compress]") {
    Compressor c;
    std::string content =
        "// top comment\n"
        "void foo(int x) {\n"
        "  // inner comment line 1\n"
        "  // inner comment line 2\n"
        "  do_stuff();\n"
        "}\n";

    std::string out = c.process(content, "foo.cpp", true, false, false);

    REQUIRE(out.find("void foo(int x)") != std::string::npos);
    REQUIRE(out.find("inner comment") != std::string::npos);
}
