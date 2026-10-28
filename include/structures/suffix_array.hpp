#pragma once

#include <string>
#include <vector>

class SuffixArray {
public:
    // Build the suffix array from the given normalized text.
    explicit SuffixArray(const std::string& normalizedText);

    // Return all starting offsets in the text where `pattern` occurs.
    // K2 provides the search implementation.
    std::vector<int> search(const std::string& pattern) const;

    // Number of suffixes (= text length).
    std::size_t size() const;

    // The normalized text this array was built from.
    const std::string& text() const;

private:
    std::string text_;
    std::vector<int> sa_;  // sa_[i] is the start of the i-th sorted suffix

    void buildPrefixDoubling();

    // K2 uses this to compare a suffix with a search pattern.
    int compareAtPosition(int pos, const std::string& pattern) const;
};
