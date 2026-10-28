#pragma once
#include "models/search_result.hpp"
#include <vector>
#include <cstddef>

struct SearchResultComparator {
    bool operator()(const SearchResult& a, const SearchResult& b) const;
};

class MaxHeap {
public:
    MaxHeap();
    explicit MaxHeap(std::vector<SearchResult> candidates);

    void push(const SearchResult& result);
    SearchResult pop();
    const SearchResult& top() const;

    bool empty() const;
    std::size_t size() const;

    std::vector<SearchResult> extractTopK(std::size_t k);

private:
    std::vector<SearchResult> data_;
    SearchResultComparator comp_;

    void heapifyUp(std::size_t idx);
    void heapifyDown(std::size_t idx);

    std::size_t parent(std::size_t i) const { return (i - 1) / 2; }
    std::size_t leftChild(std::size_t i) const { return 2 * i + 1; }
    std::size_t rightChild(std::size_t i) const { return 2 * i + 2; }
};
