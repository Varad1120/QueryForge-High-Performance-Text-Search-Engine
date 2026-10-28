#include "engine/search_engine.hpp"

#include "engine/normalizer.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ratio>
#include <stdexcept>
#include <utility>

namespace {

std::string normalizeText(const std::string& text) {
    const std::vector<std::string> tokens = Normalizer::tokenize(text);
    std::string normalized;
    for (const std::string& token : tokens) {
        if (!normalized.empty()) {
            normalized.push_back(' ');
        }
        normalized.append(token);
    }
    return normalized;
}

}  // namespace

SearchEngine::SearchEngine(DocumentRepository& repo)
    : repo_(repo), avlTree_(std::make_unique<AVLTree>()) {}

void SearchEngine::buildIndexes() {
    const IndexState previousState = state_;
    if (previousState == IndexState::READY) {
        state_ = IndexState::STALE;
    }

    try {
        const std::vector<Document> documents = repo_.getAllDocuments();

        // Stage the complete replacement before publishing any of its indexes.
        SearchEngine staged(repo_);
        for (const Document& document : documents) {
            staged.indexDocument(document);
        }

        trie_ = std::move(staged.trie_);
        suffixArrays_.swap(staged.suffixArrays_);
        avlTree_.swap(staged.avlTree_);
        documentCache_.swap(staged.documentCache_);
        state_ = IndexState::READY;
    } catch (...) {
        state_ = previousState == IndexState::NOT_BUILT
                     ? IndexState::NOT_BUILT
                     : IndexState::STALE;
        throw;
    }
}

std::size_t SearchEngine::documentCount() const {
    return documentCache_.size();
}

std::vector<TrieSuggestion> SearchEngine::autocomplete(const std::string& prefix,
                                                       std::size_t maxSuggestions) {
    if (state_ != IndexState::READY) {
        throw std::logic_error("Cannot autocomplete before indexes are ready");
    }

    const std::string normalizedPrefix = Normalizer::normalizeToken(prefix);
    return trie_.autocomplete(normalizedPrefix, maxSuggestions);
}

std::vector<long long> SearchEngine::rangeQuery(long long fromId, long long toId) {
    if (state_ != IndexState::READY) {
        throw std::logic_error("Cannot query document ranges before indexes are ready");
    }
    if (fromId > toId) {
        return {};
    }

    return avlTree_->rangeQuery(fromId, toId);
}

std::vector<SearchResult> SearchEngine::search(const std::string& query, int limit) {
    if (state_ != IndexState::READY) {
        throw std::logic_error("Cannot search before indexes are ready");
    }
    if (limit <= 0) {
        throw std::invalid_argument("Search result limit must be positive");
    }

    const auto startedAt = std::chrono::steady_clock::now();
    const std::string normalizedQuery = normalizeText(query);
    const auto scoringTime = std::chrono::system_clock::now();
    std::vector<SearchResult> candidates;

    if (!normalizedQuery.empty()) {
        candidates.reserve(suffixArrays_.size());
        for (const auto& [documentId, suffixArray] : suffixArrays_) {
            const std::vector<int> matches = suffixArray.search(normalizedQuery);
            if (matches.empty()) {
                continue;
            }

            const auto documentIt = documentCache_.find(documentId);
            if (documentIt == documentCache_.end()) {
                throw std::logic_error("Indexed document metadata is missing");
            }

            SearchResult result;
            result.documentId = documentId;
            result.title = documentIt->second.title;
            result.snippet = extractSnippet(documentIt->second.body, normalizedQuery);
            result.matchCount = static_cast<int>(matches.size());
            result.score = computeScore(documentId,
                                        normalizedQuery,
                                        result.matchCount,
                                        scoringTime);
            candidates.push_back(std::move(result));
        }
    }

    MaxHeap rankingHeap(std::move(candidates));
    std::vector<SearchResult> results =
        rankingHeap.extractTopK(static_cast<std::size_t>(limit));

    SearchHistoryEntry history;
    history.queryText = query;
    history.queryType = "substring";
    history.requestedLimit = limit;
    history.resultCount = static_cast<int>(results.size());
    history.elapsedMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::steady_clock::now() - startedAt)
                                .count();
    repo_.insertSearchHistory(history);

    return results;
}

double SearchEngine::computeScore(long long docId,
                                  const std::string& normalizedQuery,
                                  int matchCount,
                                  std::chrono::system_clock::time_point scoringTime) const {
    const auto documentIt = documentCache_.find(docId);
    if (documentIt == documentCache_.end()) {
        throw std::logic_error("Cannot score a document absent from the metadata cache");
    }

    const std::string normalizedTitle = normalizeText(documentIt->second.title);
    const double titleMatch = normalizedTitle.find(normalizedQuery) == std::string::npos
                                  ? 0.0
                                  : 1.0;

    const double daysSinceCreation = std::chrono::duration<double, std::ratio<86400>>(
                                         scoringTime - documentIt->second.createdAt)
                                         .count();
    const double recencyFactor = std::clamp(
        1.0 - daysSinceCreation / kRecencyWindowDays, 0.0, 1.0);

    return kTitleMatchWeight * titleMatch +
           kBodyOccurrenceWeight * std::log1p(static_cast<double>(matchCount)) +
           kRecencyWeight * recencyFactor;
}

std::string SearchEngine::extractSnippet(const std::string& body,
                                         const std::string& query,
                                         int maxLength) const {
    if (maxLength <= 0) {
        return {};
    }

    const std::string normalizedBody = normalizeText(body);
    if (normalizedBody.empty()) {
        return {};
    }

    const std::size_t capacity = static_cast<std::size_t>(maxLength);
    if (normalizedBody.size() <= capacity) {
        return normalizedBody;
    }

    const std::size_t matchPosition = normalizedBody.find(query);
    const std::size_t firstMatch = matchPosition == std::string::npos ? 0 : matchPosition;
    const std::size_t contextBefore = capacity / 3;
    std::size_t start = firstMatch > contextBefore
                            ? firstMatch - contextBefore
                            : 0;
    start = std::min(start, normalizedBody.size() - capacity);
    const std::size_t length = std::min(capacity, normalizedBody.size() - start);
    return normalizedBody.substr(start, length);
}

void SearchEngine::indexDocument(const Document& doc) {
    if (doc.id <= 0) {
        throw std::invalid_argument("Cannot index a document with a non-positive ID");
    }
    if (documentCache_.find(doc.id) != documentCache_.end()) {
        throw std::invalid_argument("Cannot index duplicate document ID " +
                                    std::to_string(doc.id));
    }

    const std::vector<std::string> titleTokens = Normalizer::tokenize(doc.title);
    const std::vector<std::string> bodyTokens = Normalizer::tokenize(doc.body);

    for (const std::string& term : titleTokens) {
        trie_.insertTerm(term, doc.id);
    }
    for (const std::string& term : bodyTokens) {
        trie_.insertTerm(term, doc.id);
    }

    std::string normalizedBody;
    for (const std::string& token : bodyTokens) {
        if (!normalizedBody.empty()) {
            normalizedBody.push_back(' ');
        }
        normalizedBody.append(token);
    }

    suffixArrays_.emplace(doc.id, SuffixArray(normalizedBody));
    avlTree_->insert(doc.id);
    documentCache_.emplace(doc.id, doc);
}
