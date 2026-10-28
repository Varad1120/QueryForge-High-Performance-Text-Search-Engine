#include "structures/suffix_array.hpp"

#include <algorithm>
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
