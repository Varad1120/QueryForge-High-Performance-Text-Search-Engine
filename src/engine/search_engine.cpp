#include "engine/search_engine.hpp"

#include "engine/normalizer.hpp"

#include <stdexcept>
#include <utility>

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
