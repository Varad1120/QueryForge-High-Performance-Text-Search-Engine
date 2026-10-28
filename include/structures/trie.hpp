#pragma once
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

using namespace std;

struct TrieSuggestion {
    string term;
    size_t documentFreq{0};

    bool operator==(const TrieSuggestion& other) const {
        return term == other.term && documentFreq == other.documentFreq;
    }
};

struct TrieNode {
    unordered_map<char, unique_ptr<TrieNode>> children;
    bool                  isTerminal{false};
    size_t                documentFreq{0};   // number of distinct docs containing this term
    size_t                termFreq{0};       // total occurrences across all docs
    unordered_set<long long> documentIds;     // distinct document IDs
};

class Trie {
public:
    Trie();
    ~Trie() = default;

    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;
    Trie(Trie&&) noexcept = default;
    Trie& operator=(Trie&&) noexcept = default;

    // Insert a normalized term for a given document.
    void insertTerm(const string& term, long long documentId);

    // Return up to maxSuggestions completions for the given normalized prefix,
    // sorted by documentFreq descending.
    vector<TrieSuggestion> autocomplete(
        const string& normalizedPrefix,
        size_t maxSuggestions = 10) const;

    // Return true if the exact term exists in the Trie.
    bool contains(const string& term) const;

    size_t uniqueTermCount() const;

private:
    unique_ptr<TrieNode> root_;
    size_t               uniqueTerms_{0};

    TrieNode* findNode(const string& prefix) const;
    void collectSuggestions(const TrieNode* node,
                            const string& prefix,
                            vector<TrieSuggestion>& out) const;
};
