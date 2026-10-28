#include "structures/suffix_array.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <stdexcept>

SuffixArray::SuffixArray(const std::string& normalizedText) {
    if (normalizedText.size() >
        static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("SuffixArray text exceeds supported index range");
    }

    text_ = normalizedText;
    buildPrefixDoubling();
}

std::size_t SuffixArray::size() const {
    return sa_.size();
}

const std::string& SuffixArray::text() const {
    return text_;
}

std::vector<int> SuffixArray::search(const std::string& pattern) const {
    if (text_.empty() || pattern.empty() || pattern.size() > text_.size()) {
        return {};
    }

    std::size_t first = 0;
    std::size_t last = sa_.size();
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (compareAtPosition(sa_[middle], pattern) < 0) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    const std::size_t matchBegin = first;

    if (matchBegin == sa_.size() ||
        compareAtPosition(sa_[matchBegin], pattern) != 0) {
        return {};
    }

    last = sa_.size();
    while (first < last) {
        const std::size_t middle = first + (last - first) / 2;
        if (compareAtPosition(sa_[middle], pattern) <= 0) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }

    return std::vector<int>(sa_.begin() + static_cast<std::ptrdiff_t>(matchBegin),
                            sa_.begin() + static_cast<std::ptrdiff_t>(first));
}

int SuffixArray::compareAtPosition(int pos, const std::string& pattern) const {
    if (pos < 0 || static_cast<std::size_t>(pos) >= text_.size()) {
        return -1;
    }

    const std::size_t suffixStart = static_cast<std::size_t>(pos);
    const std::size_t suffixLength = text_.size() - suffixStart;
    const std::size_t comparedLength = std::min(suffixLength, pattern.size());
    for (std::size_t i = 0; i < comparedLength; ++i) {
        const auto suffixByte = static_cast<unsigned char>(text_[suffixStart + i]);
        const auto patternByte = static_cast<unsigned char>(pattern[i]);
        if (suffixByte < patternByte) {
            return -1;
        }
        if (suffixByte > patternByte) {
            return 1;
        }
    }

    if (comparedLength == pattern.size()) {
        return 0;
    }
    return -1;
}

void SuffixArray::buildPrefixDoubling() {
    const std::size_t length = text_.size();
    if (length == 0) {
        return;
    }

    sa_.resize(length);
    std::iota(sa_.begin(), sa_.end(), 0);

    std::vector<int> ranks(length);
    for (std::size_t i = 0; i < length; ++i) {
        ranks[i] = static_cast<unsigned char>(text_[i]);
    }

    std::vector<int> nextRanks(length);
    std::size_t gap = 1;
    std::size_t classes = 0;

    while (classes < length) {
        const auto secondRank = [&ranks, length, gap](int suffixStart) {
            const std::size_t start = static_cast<std::size_t>(suffixStart);
            if (gap >= length - start) {
                return -1;
            }
            return ranks[start + gap];
        };

        std::sort(sa_.begin(), sa_.end(), [&ranks, &secondRank](int left, int right) {
            const std::size_t leftIndex = static_cast<std::size_t>(left);
            const std::size_t rightIndex = static_cast<std::size_t>(right);
            if (ranks[leftIndex] != ranks[rightIndex]) {
                return ranks[leftIndex] < ranks[rightIndex];
            }
            return secondRank(left) < secondRank(right);
        });

        classes = 1;
        nextRanks[static_cast<std::size_t>(sa_[0])] = 0;
        for (std::size_t i = 1; i < length; ++i) {
            const int previous = sa_[i - 1];
            const int current = sa_[i];
            const std::size_t previousIndex = static_cast<std::size_t>(previous);
            const std::size_t currentIndex = static_cast<std::size_t>(current);

            if (ranks[previousIndex] != ranks[currentIndex] ||
                secondRank(previous) != secondRank(current)) {
                ++classes;
            }
            nextRanks[currentIndex] = static_cast<int>(classes - 1);
        }

        ranks.swap(nextRanks);
        if (classes == length) {
            break;
        }

        // Saturate at length rather than multiplying past size_t's range.
        gap = gap > length / 2 ? length : gap * 2;
    }
}
