#include <catch2/catch_test_macros.hpp>
#include "structures/max_heap.hpp"
#include <algorithm>
#include <random>
#include <vector>

static SearchResult makeResult(long long docId, double score) {
    SearchResult r;
    r.documentId = docId;
    r.title = "Doc " + std::to_string(docId);
    r.snippet = "Snippet";
    r.score = score;
    r.matchCount = 1;
    return r;
}

TEST_CASE("MaxHeap push with increasing scores extracts in descending order", "[max_heap]") {
    MaxHeap heap;
    heap.push(makeResult(1, 10.0));
    heap.push(makeResult(2, 20.0));
    heap.push(makeResult(3, 30.0));
    heap.push(makeResult(4, 40.0));
    heap.push(makeResult(5, 50.0));

    auto top3 = heap.extractTopK(3);
    REQUIRE(top3.size() == 3);
    REQUIRE(top3[0].documentId == 5);
    REQUIRE(top3[0].score == 50.0);
    REQUIRE(top3[1].documentId == 4);
    REQUIRE(top3[1].score == 40.0);
    REQUIRE(top3[2].documentId == 3);
    REQUIRE(top3[2].score == 30.0);

    REQUIRE(heap.empty());
}

TEST_CASE("MaxHeap push with decreasing scores extracts in descending order", "[max_heap]") {
    MaxHeap heap;
    heap.push(makeResult(5, 50.0));
    heap.push(makeResult(4, 40.0));
    heap.push(makeResult(3, 30.0));
    heap.push(makeResult(2, 20.0));
    heap.push(makeResult(1, 10.0));

    auto top3 = heap.extractTopK(3);
    REQUIRE(top3.size() == 3);
    REQUIRE(top3[0].documentId == 5);
    REQUIRE(top3[1].documentId == 4);
    REQUIRE(top3[2].documentId == 3);

    REQUIRE(heap.empty());
}

TEST_CASE("MaxHeap breaks ties deterministically by ascending documentId", "[max_heap]") {
    MaxHeap heap;
    heap.push(makeResult(10, 25.0));
    heap.push(makeResult(5, 25.0));
    heap.push(makeResult(20, 25.0));
    heap.push(makeResult(1, 25.0));

    auto results = heap.extractTopK(4);
    REQUIRE(results.size() == 4);
    REQUIRE(results[0].documentId == 1);
    REQUIRE(results[1].documentId == 5);
    REQUIRE(results[2].documentId == 10);
    REQUIRE(results[3].documentId == 20);
}

TEST_CASE("MaxHeap extractTopK with K larger than heap size returns all elements", "[max_heap]") {
    MaxHeap heap;
    heap.push(makeResult(1, 5.0));
    heap.push(makeResult(2, 15.0));

    auto results = heap.extractTopK(10);
    REQUIRE(results.size() == 2);
    REQUIRE(results[0].documentId == 2);
    REQUIRE(results[1].documentId == 1);
    REQUIRE(heap.empty());
}

TEST_CASE("MaxHeap extractTopK with K=0 returns empty vector", "[max_heap]") {
    MaxHeap heap;
    heap.push(makeResult(1, 10.0));
    heap.push(makeResult(2, 20.0));

    auto results = heap.extractTopK(0);
    REQUIRE(results.empty());
    REQUIRE(heap.empty());
}

TEST_CASE("MaxHeap operations on empty heap", "[max_heap]") {
    MaxHeap heap;
    REQUIRE(heap.empty());
    REQUIRE(heap.size() == 0);
    REQUIRE_THROWS_AS(heap.pop(), std::runtime_error);
    REQUIRE_THROWS_AS(heap.top(), std::runtime_error);

    auto results = heap.extractTopK(5);
    REQUIRE(results.empty());
}

TEST_CASE("MaxHeap vector constructor builds in O(M) with identical order", "[max_heap]") {
    std::vector<SearchResult> items = {
        makeResult(1, 10.0),
        makeResult(2, 50.0),
        makeResult(3, 30.0),
        makeResult(4, 20.0),
        makeResult(5, 40.0)
    };

    MaxHeap bulkHeap(items);
    REQUIRE(bulkHeap.size() == 5);

    MaxHeap pushHeap;
    for (const auto& item : items) {
        pushHeap.push(item);
    }

    auto bulkResults = bulkHeap.extractTopK(5);
    auto pushResults = pushHeap.extractTopK(5);

    REQUIRE(bulkResults.size() == 5);
    REQUIRE(pushResults.size() == 5);
    for (std::size_t i = 0; i < 5; ++i) {
        REQUIRE(bulkResults[i].documentId == pushResults[i].documentId);
        REQUIRE(bulkResults[i].score == pushResults[i].score);
    }
}

TEST_CASE("MaxHeap differential test against std::sort for random inputs", "[max_heap]") {
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> scoreDist(0.0, 100.0);
    std::uniform_int_distribution<long long> idDist(1, 500);

    auto sortDesc = [](const SearchResult& a, const SearchResult& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.documentId < b.documentId;
    };

    for (int run = 0; run < 30; ++run) {
        std::size_t n = 20 + (run % 30);
        std::vector<SearchResult> input;
        for (std::size_t i = 0; i < n; ++i) {
            input.push_back(makeResult(idDist(rng), scoreDist(rng)));
        }

        std::vector<SearchResult> expected = input;
        std::sort(expected.begin(), expected.end(), sortDesc);

        MaxHeap heap(input);
        std::size_t k = (run % 15) + 1;
        auto actual = heap.extractTopK(k);

        std::size_t expectedCount = std::min(k, n);
        REQUIRE(actual.size() == expectedCount);
        for (std::size_t i = 0; i < expectedCount; ++i) {
            REQUIRE(actual[i].documentId == expected[i].documentId);
            REQUIRE(actual[i].score == expected[i].score);
        }
    }
}
