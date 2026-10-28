#include "structures/max_heap.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

bool SearchResultComparator::operator()(const SearchResult& a, const SearchResult& b) const {
    if (a.score != b.score) {
        return a.score < b.score;
    }
    return a.documentId > b.documentId;
}

MaxHeap::MaxHeap() = default;

MaxHeap::MaxHeap(std::vector<SearchResult> candidates)
    : data_(std::move(candidates)) {
    if (!data_.empty()) {
        for (long long i = static_cast<long long>(data_.size() / 2) - 1; i >= 0; --i) {
            heapifyDown(static_cast<std::size_t>(i));
        }
    }
}

void MaxHeap::push(const SearchResult& result) {
    data_.push_back(result);
    heapifyUp(data_.size() - 1);
}

SearchResult MaxHeap::pop() {
    if (data_.empty()) {
        throw std::runtime_error("MaxHeap::pop called on empty heap");
    }
    SearchResult topElem = std::move(data_.front());
    if (data_.size() == 1) {
        data_.pop_back();
    } else {
        data_.front() = std::move(data_.back());
        data_.pop_back();
        heapifyDown(0);
    }
    return topElem;
}

const SearchResult& MaxHeap::top() const {
    if (data_.empty()) {
        throw std::runtime_error("MaxHeap::top called on empty heap");
    }
    return data_.front();
}

bool MaxHeap::empty() const {
    return data_.empty();
}

std::size_t MaxHeap::size() const {
    return data_.size();
}

std::vector<SearchResult> MaxHeap::extractTopK(std::size_t k) {
    std::vector<SearchResult> results;
    if (k == 0 || data_.empty()) {
        data_.clear();
        return results;
    }

    results.reserve(std::min(k, data_.size()));
    while (!data_.empty() && results.size() < k) {
        results.push_back(pop());
    }
    data_.clear();
    return results;
}

void MaxHeap::heapifyUp(std::size_t idx) {
    while (idx > 0) {
        std::size_t p = parent(idx);
        if (comp_(data_[p], data_[idx])) {
            std::swap(data_[p], data_[idx]);
            idx = p;
        } else {
            break;
        }
    }
}

void MaxHeap::heapifyDown(std::size_t idx) {
    std::size_t n = data_.size();
    while (true) {
        std::size_t largest = idx;
        std::size_t left = leftChild(idx);
        std::size_t right = rightChild(idx);

        if (left < n && comp_(data_[largest], data_[left])) {
            largest = left;
        }
        if (right < n && comp_(data_[largest], data_[right])) {
            largest = right;
        }

        if (largest != idx) {
            std::swap(data_[idx], data_[largest]);
            idx = largest;
        } else {
            break;
        }
    }
}
