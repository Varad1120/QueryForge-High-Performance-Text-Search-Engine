#include <catch2/catch_test_macros.hpp>
#include "structures/trie.hpp"
#include <algorithm>
#include <random>
#include <map>

using namespace std;

TEST_CASE("Trie: Insert and find a single term", "[trie]") {
    Trie trie;
    REQUIRE_FALSE(trie.contains("algorithm"));
    REQUIRE(trie.uniqueTermCount() == 0);

    trie.insertTerm("algorithm", 1);
    REQUIRE(trie.contains("algorithm"));
    REQUIRE_FALSE(trie.contains("alg"));
    REQUIRE_FALSE(trie.contains("algorithms"));
    REQUIRE(trie.uniqueTermCount() == 1);
}

TEST_CASE("Trie: Prefix with exactly one completion", "[trie]") {
    Trie trie;
    trie.insertTerm("datastructure", 1);
    trie.insertTerm("database", 2);

    auto suggestions = trie.autocomplete("datas");
    REQUIRE(suggestions.size() == 1);
    REQUIRE(suggestions[0].term == "datastructure");
    REQUIRE(suggestions[0].documentFreq == 1);
}

TEST_CASE("Trie: Prefix with multiple completions sorted by frequency descending", "[trie]") {
    Trie trie;
    // "binary" in 3 documents
    trie.insertTerm("binary", 1);
    trie.insertTerm("binary", 2);
    trie.insertTerm("binary", 3);

    // "binomial" in 1 document
    trie.insertTerm("binomial", 1);

    // "bin" in 2 documents
    trie.insertTerm("bin", 1);
    trie.insertTerm("bin", 4);

    auto results = trie.autocomplete("bin", 10);
    REQUIRE(results.size() == 3);
    REQUIRE(results[0].term == "binary");
    REQUIRE(results[0].documentFreq == 3);
    REQUIRE(results[1].term == "bin");
    REQUIRE(results[1].documentFreq == 2);
    REQUIRE(results[2].term == "binomial");
    REQUIRE(results[2].documentFreq == 1);
}

TEST_CASE("Trie: Prefix absent from Trie returns empty vector", "[trie]") {
    Trie trie;
    trie.insertTerm("compiler", 1);
    trie.insertTerm("computation", 2);

    auto results = trie.autocomplete("xyz");
    REQUIRE(results.empty());
}

TEST_CASE("Trie: Duplicate insertion from same document does not increment documentFreq", "[trie]") {
    Trie trie;
    trie.insertTerm("query", 10);
    trie.insertTerm("query", 10);
    trie.insertTerm("query", 10);

    auto suggestions = trie.autocomplete("query");
    REQUIRE(suggestions.size() == 1);
    REQUIRE(suggestions[0].documentFreq == 1);

    // Now insert from another document
    trie.insertTerm("query", 20);
    suggestions = trie.autocomplete("query");
    REQUIRE(suggestions[0].documentFreq == 2);
}

TEST_CASE("Trie: Delete a document sharing a prefix leaves other document terms intact", "[trie]") {
    Trie trie;
    trie.insertTerm("tree", 1);
    trie.insertTerm("trie", 2);

    REQUIRE(trie.contains("tree"));
    REQUIRE(trie.contains("trie"));
    REQUIRE(trie.uniqueTermCount() == 2);

    trie.removeDocument(1);
    REQUIRE_FALSE(trie.contains("tree"));
    REQUIRE(trie.contains("trie"));
    REQUIRE(trie.uniqueTermCount() == 1);
}

TEST_CASE("Trie: Empty prefix returns completions capped at maxSuggestions", "[trie]") {
    Trie trie;
    vector<string> words = {"apple", "banana", "cherry", "date", "fig", "grape"};
    for (size_t i = 0; i < words.size(); ++i) {
        trie.insertTerm(words[i], static_cast<long long>(i + 1));
    }

    auto results = trie.autocomplete("", 3);
    REQUIRE(results.size() == 3);

    auto allResults = trie.autocomplete("", 10);
    REQUIRE(allResults.size() == words.size());
}

TEST_CASE("Trie: removeDocument removes all terms belonging to that document", "[trie]") {
    Trie trie;
    trie.insertTerm("doc1_alpha", 1);
    trie.insertTerm("doc1_beta", 1);
    trie.insertTerm("shared_gamma", 1);
    trie.insertTerm("shared_gamma", 2);
    trie.insertTerm("doc2_delta", 2);

    REQUIRE(trie.uniqueTermCount() == 4);

    trie.removeDocument(1);

    REQUIRE_FALSE(trie.contains("doc1_alpha"));
    REQUIRE_FALSE(trie.contains("doc1_beta"));
    REQUIRE(trie.contains("shared_gamma"));
    REQUIRE(trie.contains("doc2_delta"));
    REQUIRE(trie.uniqueTermCount() == 2);

    auto gamma = trie.autocomplete("shared_gamma");
    REQUIRE(gamma.size() == 1);
    REQUIRE(gamma[0].documentFreq == 1);
}

TEST_CASE("Trie: Differential test against linear scan", "[trie]") {
    Trie trie;
    map<string, unordered_set<long long>> reference;

    vector<string> vocabulary = {
        "search", "sorting", "suffix", "tree", "trie", "table", "structure",
        "scale", "scope", "scan", "stack", "string", "subgraph", "subset"
    };

    mt19937 rng(42);
    uniform_int_distribution<size_t> wordDist(0, vocabulary.size() - 1);
    uniform_int_distribution<long long> docDist(1, 5);

    // Insert 100 random term-document pairs
    for (int i = 0; i < 100; ++i) {
        const string& term = vocabulary[wordDist(rng)];
        long long docId = docDist(rng);
        trie.insertTerm(term, docId);
        reference[term].insert(docId);
    }

    // Verify contains and documentFreq for all vocabulary terms
    for (const auto& term : vocabulary) {
        bool inRef = (reference.find(term) != reference.end() && !reference[term].empty());
        REQUIRE(trie.contains(term) == inRef);
        if (inRef) {
            auto suggestions = trie.autocomplete(term, 1);
            REQUIRE(!suggestions.empty());
            REQUIRE(suggestions[0].documentFreq == reference[term].size());
        }
    }

    // Differential test for prefixes
    vector<string> prefixes = {"s", "sc", "su", "st", "tr", "z"};
    for (const auto& prefix : prefixes) {
        vector<TrieSuggestion> refSuggestions;
        for (const auto& [term, docs] : reference) {
            if (term.rfind(prefix, 0) == 0 && !docs.empty()) {
                refSuggestions.push_back({term, docs.size()});
            }
        }
        sort(refSuggestions.begin(), refSuggestions.end(), [](const TrieSuggestion& a, const TrieSuggestion& b) {
            if (a.documentFreq != b.documentFreq) return a.documentFreq > b.documentFreq;
            return a.term < b.term;
        });
        if (refSuggestions.size() > 5) refSuggestions.resize(5);

        auto trieSuggestions = trie.autocomplete(prefix, 5);
        REQUIRE(trieSuggestions == refSuggestions);
    }
}
