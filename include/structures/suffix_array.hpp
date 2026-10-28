#pragma once

#include <string>
#include <vector>

class SuffixArray {
public:
    // Build the suffix array from the given normalized text.
    explicit SuffixArray(const std::string& normalizedText);

    // Return all starting offsets where `pattern` occurs, in suffix-array
    // order (lexicographic order of the matching suffixes). Empty patterns
    // and patterns with no match return an empty vector.
    std::vector<int> search(const std::string& pattern) const;

    // Number of suffixes (= text length).
    std::size_t size() const;

    // The normalized text this array was built from.
    const std::string& text() const;

private:
    std::string text_;
    std::vector<int> sa_;  // sa_[i] is the start of the i-th sorted suffix

    void buildPrefixDoubling();

    // Compare the suffix at pos with pattern; zero means pattern is a prefix
    // of the suffix.
    int compareAtPosition(int pos, const std::string& pattern) const;
};
