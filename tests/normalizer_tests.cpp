#include <catch2/catch_test_macros.hpp>
#include "engine/normalizer.hpp"

using namespace std;

TEST_CASE("Normalizer::tokenize handles empty and whitespace inputs", "[normalizer]") {
    REQUIRE(Normalizer::tokenize("").empty());
    REQUIRE(Normalizer::tokenize("   ").empty());
    REQUIRE(Normalizer::tokenize("\t\n\r  \v").empty());
}

TEST_CASE("Normalizer::tokenize strips punctuation and symbols", "[normalizer]") {
    REQUIRE(Normalizer::tokenize("!@#$%^&*()_+-=[]{}|;':\",./<>?").empty());
    
    vector<string> expected = {"hello", "world"};
    REQUIRE(Normalizer::tokenize("...hello...world...") == expected);
    REQUIRE(Normalizer::tokenize("hello, world!") == expected);
}

TEST_CASE("Normalizer::tokenize converts uppercase to lowercase", "[normalizer]") {
    vector<string> expected = {"queryforge", "text", "search"};
    REQUIRE(Normalizer::tokenize("QueryForge TEXT Search") == expected);
}

TEST_CASE("Normalizer::tokenize preserves numbers and alphanumeric tokens", "[normalizer]") {
    vector<string> expected = {"v2", "release", "100", "percent"};
    REQUIRE(Normalizer::tokenize("v2 release 100 percent") == expected);
}

TEST_CASE("Normalizer::tokenize handles complex sentence", "[normalizer]") {
    string text = "Advanced Data Structures (ADS) - Mini-Project 2026: High-Performance!";
    vector<string> expected = {
        "advanced", "data", "structures", "ads", "mini", "project", "2026", "high", "performance"
    };
    REQUIRE(Normalizer::tokenize(text) == expected);
}

TEST_CASE("Normalizer::normalizeToken lowers and cleans token", "[normalizer]") {
    REQUIRE(Normalizer::normalizeToken("") == "");
    REQUIRE(Normalizer::normalizeToken("Algorithm") == "algorithm");
    REQUIRE(Normalizer::normalizeToken("TRIE") == "trie");
    REQUIRE(Normalizer::normalizeToken("c++") == "c");
    REQUIRE(Normalizer::normalizeToken("v2.0") == "v20");
    REQUIRE(Normalizer::normalizeToken("!!!") == "");
}
