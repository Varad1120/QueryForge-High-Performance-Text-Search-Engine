#pragma once

#include "models/document.hpp"
#include "models/search_result.hpp"
#include "repository/document_repository.hpp"
#include "structures/avl_tree.hpp"
#include "structures/max_heap.hpp"
#include "structures/suffix_array.hpp"
#include "structures/trie.hpp"

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class IndexState { NOT_BUILT, READY, STALE };

class SearchEngine {
public:
    explicit SearchEngine(DocumentRepository& repo);

    // Load all documents from the repository and build all in-memory indexes.
    void buildIndexes();

    std::vector<TrieSuggestion> autocomplete(
        const std::string& prefix,
        std::size_t maxSuggestions = 10);

    // Search requires READY indexes and a positive limit. Empty normalized
    // queries return no results; valid calls are recorded in search history.
    std::vector<SearchResult> search(const std::string& query, int limit = 10);
    std::vector<long long> rangeQuery(long long fromId, long long toId);

    long long addDocument(Document doc);
    void updateDocument(Document doc);
    void deleteDocument(long long id);

    IndexState indexState() const { return state_; }
    std::size_t documentCount() const;

private:
    // LLD relevance score: 5*title match + log1p(body occurrences)
    // + 0.5*recency, where recency decays to zero over one year.
    static constexpr double kTitleMatchWeight = 5.0;
    static constexpr double kBodyOccurrenceWeight = 1.0;
    static constexpr double kRecencyWeight = 0.5;
    static constexpr double kRecencyWindowDays = 365.0;

    DocumentRepository& repo_;

    Trie trie_;
    std::unordered_map<long long, SuffixArray> suffixArrays_;
    // Swapping a fully built tree keeps failed rebuilds from exposing partial indexes.
    std::unique_ptr<AVLTree> avlTree_;
    IndexState state_{IndexState::NOT_BUILT};
    std::unordered_map<long long, Document> documentCache_;

    void indexDocument(const Document& doc);
    void deindexDocument(long long id);

    double computeScore(long long docId,
                        const std::string& normalizedQuery,
                        int matchCount,
                        std::chrono::system_clock::time_point scoringTime) const;
    std::string extractSnippet(const std::string& body,
                               const std::string& query,
                               int maxLength = 180) const;
};
