#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../src/utils.h"
#include <string>
#include <vector>

using namespace rcpack;

TEST_CASE("trim: basic trimming behavior", "[trim]") {
    REQUIRE(trim("") == "");
    REQUIRE(trim("   ") == "");
    REQUIRE(trim("  a  ") == "a");
    REQUIRE(trim("\t\n  hello \r\n") == "hello");
}

TEST_CASE("normalize_extension_token: wildcard, dot, alnum and quoted", "[normalize_extension_token]") {
    REQUIRE(normalize_extension_token("*.js") == ".js");
    REQUIRE(normalize_extension_token(".CPP") == ".cpp");
    REQUIRE(normalize_extension_token("js") == ".js");
    REQUIRE(normalize_extension_token("'*.h'") == ".h");
    REQUIRE(normalize_extension_token("\"cpp\"") == ".cpp");
    REQUIRE(normalize_extension_token("not*good") == "");
}

TEST_CASE("find_matching_brace_safe: nested braces with strings and comments", "[find_matching_brace_safe]") {
    std::string s = "void f() { int x = 0; /* comment { not brace */ std::string t = \"}\"; if (x) { return; } }";
    size_t open = s.find('{');
    REQUIRE(open != std::string::npos);
    size_t match = find_matching_brace_safe(s, open);
    REQUIRE(match != std::string::npos);
    REQUIRE(s[match] == '}');

    std::string inside = s.substr(open + 1, match - open - 1);
    REQUIRE(inside.find("return;") != std::string::npos);
}

TEST_CASE("collect_comments_in_range extracts // and /* */ inside range", "[collect_comments_in_range]") {
    std::string s = "function() { // line comment\n int x = 1; /* block comment */ std::string s = \"/* not comment */\"; // another\n }";
    size_t start = s.find('{');
    size_t end = s.find('}', start);
    REQUIRE(start != std::string::npos);
    REQUIRE(end != std::string::npos);
    auto res = collect_comments_in_range(s, start, end);

    REQUIRE(res.find("// line comment") != std::string::npos);
    REQUIRE(res.find("/* block comment */") != std::string::npos);

    REQUIRE(res.find("not comment") == std::string::npos);
}

TEST_CASE("collect_preceding_comment_block_lines returns contiguous comment block", "[collect_preceding_comment_block_lines]") {
    std::vector<std::string> lines = {
        "// Header comment line 1",
        "// Header comment line 2",
        "",
        "void foo() {",
        "  // body",
        "}"
    };

    auto res = collect_preceding_comment_block_lines(lines, 3); // index of "void foo() {"
    REQUIRE(!res.empty());
    REQUIRE(res.find("Header comment line 1") != std::string::npos);
    REQUIRE(res.find("Header comment line 2") != std::string::npos);
}
