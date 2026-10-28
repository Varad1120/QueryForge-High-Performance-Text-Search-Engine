#include <catch2/catch_test_macros.hpp>

#include "structures/suffix_array.hpp"

#include <algorithm>
#include <random>
#include <string>
#include <vector>

namespace {

std::vector<int> naiveMatchesInSuffixOrder(const std::string& text,
                                           const std::string& pattern) {
    std::vector<int> matches;
    if (pattern.empty()) {
        return matches;
    }

    for (std::size_t position = text.find(pattern);
         position != std::string::npos;
         position = text.find(pattern, position + 1)) {
        matches.push_back(static_cast<int>(position));
    }

    std::sort(matches.begin(), matches.end(), [&text](int left, int right) {
        return text.substr(static_cast<std::size_t>(left)) <
               text.substr(static_cast<std::size_t>(right));
    });
    return matches;
}

}  // namespace

TEST_CASE("SuffixArray retains its input and reports suffix count", "[suffix_array]") {
    const std::string text = "banana";
    SuffixArray suffixArray(text);

    REQUIRE(suffixArray.text() == text);
    REQUIRE(suffixArray.size() == text.size());
    // Matching suffixes are returned in the documented suffix-array order.
    REQUIRE(suffixArray.search("a") == std::vector<int>{5, 3, 1});
}

TEST_CASE("SuffixArray finds a pattern exactly once", "[suffix_array]") {
    REQUIRE(SuffixArray("hello world").search("hello") == std::vector<int>{0});
}

TEST_CASE("SuffixArray returns all multiple occurrences", "[suffix_array]") {
    REQUIRE(SuffixArray("abcabcabc").search("abc") ==
            std::vector<int>{6, 3, 0});
}

TEST_CASE("SuffixArray returns no matches for absent or overlong patterns", "[suffix_array]") {
    REQUIRE(SuffixArray("hello world").search("xyz").empty());
    REQUIRE(SuffixArray("hi").search("hello").empty());
}

TEST_CASE("SuffixArray includes repeated and overlapping matches", "[suffix_array]") {
    const SuffixArray suffixArray("aaaaaa");
    REQUIRE(suffixArray.search("aa") == std::vector<int>{4, 3, 2, 1, 0});
    REQUIRE(suffixArray.search("aaa") == std::vector<int>{3, 2, 1, 0});
}

TEST_CASE("SuffixArray handles empty text and empty patterns", "[suffix_array]") {
    REQUIRE(SuffixArray("").size() == 0);
    REQUIRE(SuffixArray("").search("a").empty());
    REQUIRE(SuffixArray("text").search("").empty());
}

TEST_CASE("SuffixArray finds a match at the start of the text", "[suffix_array]") {
    REQUIRE(SuffixArray("needle rest").search("needle") == std::vector<int>{0});
}

TEST_CASE("SuffixArray finds a match at the end of the text", "[suffix_array]") {
    REQUIRE(SuffixArray("a text needle").search("needle") == std::vector<int>{7});
}

TEST_CASE("SuffixArray matches std::string::find across 50 deterministic strings",
          "[suffix_array][differential]") {
    std::mt19937 random(0xC0FFEE);
    std::uniform_int_distribution<std::size_t> textLength(0, 50);
    std::uniform_int_distribution<std::size_t> patternLength(0, 8);
    std::uniform_int_distribution<int> letter(0, 3);

    for (int run = 0; run < 50; ++run) {
        const std::size_t length = textLength(random);
        std::string text(length, 'a');
        for (char& character : text) {
            character = static_cast<char>('a' + letter(random));
        }

        std::string pattern;
        if (length > 0 && run % 2 == 0) {
            std::uniform_int_distribution<std::size_t> startDistribution(0, length - 1);
            const std::size_t start = startDistribution(random);
            const std::size_t maxPatternLength = std::min<std::size_t>(8, length - start);
            std::uniform_int_distribution<std::size_t> matchLength(1, maxPatternLength);
            pattern = text.substr(start, matchLength(random));
        } else {
            pattern.resize(patternLength(random));
            for (char& character : pattern) {
                character = static_cast<char>('a' + letter(random));
            }
        }

        const SuffixArray suffixArray(text);
        REQUIRE(suffixArray.search(pattern) == naiveMatchesInSuffixOrder(text, pattern));
    }
}
