#include "structures/trie.hpp"
#include <algorithm>

using namespace std;

Trie::Trie() : root_(make_unique<TrieNode>()), uniqueTerms_(0) {}

void Trie::insertTerm(const string& term, long long documentId) {
    if (term.empty()) {
        return;
    }

    TrieNode* current = root_.get();
    for (char c : term) {
        auto it = current->children.find(c);
        if (it == current->children.end()) {
            auto inserted = current->children.emplace(c, make_unique<TrieNode>());
            current = inserted.first->second.get();
        } else {
            current = it->second.get();
        }
    }

    if (!current->isTerminal || current->documentFreq == 0) {
        current->isTerminal = true;
        ++uniqueTerms_;
    }

    current->termFreq++;
    if (current->documentIds.insert(documentId).second) {
        current->documentFreq = current->documentIds.size();
    }
}

bool Trie::contains(const string& term) const {
    if (term.empty()) {
        return false;
    }

    TrieNode* node = findNode(term);
    return node != nullptr && node->isTerminal && node->documentFreq > 0;
}

TrieNode* Trie::findNode(const string& prefix) const {
    TrieNode* current = root_.get();
    for (char c : prefix) {
        auto it = current->children.find(c);
        if (it == current->children.end()) {
            return nullptr;
        }
        current = it->second.get();
    }
    return current;
}

void Trie::collectSuggestions(const TrieNode* node,
                             const string& prefix,
                             vector<TrieSuggestion>& out) const {
    if (!node) {
        return;
    }

    if (node->isTerminal && node->documentFreq > 0) {
        out.push_back({prefix, node->documentFreq});
    }

    for (const auto& pair : node->children) {
        string nextPrefix = prefix;
        nextPrefix.push_back(pair.first);
        collectSuggestions(pair.second.get(), nextPrefix, out);
    }
}

vector<TrieSuggestion> Trie::autocomplete(
    const string& normalizedPrefix,
    size_t maxSuggestions) const {
    
    vector<TrieSuggestion> suggestions;
    if (maxSuggestions == 0) {
        return suggestions;
    }

    const TrieNode* startNode = normalizedPrefix.empty() ? root_.get() : findNode(normalizedPrefix);
    if (!startNode) {
        return suggestions;
    }

    collectSuggestions(startNode, normalizedPrefix, suggestions);

    // Sort by documentFreq descending; tie-break by term lexicographically ascending
    sort(suggestions.begin(), suggestions.end(), [](const TrieSuggestion& a, const TrieSuggestion& b) {
        if (a.documentFreq != b.documentFreq) {
            return a.documentFreq > b.documentFreq;
        }
        return a.term < b.term;
    });

    if (suggestions.size() > maxSuggestions) {
        suggestions.resize(maxSuggestions);
    }

    return suggestions;
}

void Trie::removeTerm(const string& term, long long documentId) {
    if (term.empty()) {
        return;
    }

    bool removed = false;
    removeTermHelper(root_.get(), term, 0, documentId, removed);
}

bool Trie::removeTermHelper(TrieNode* current,
                            const string& term,
                            size_t depth,
                            long long documentId,
                            bool& removed) {
    if (depth == term.size()) {
        if (!current->isTerminal || current->documentIds.find(documentId) == current->documentIds.end()) {
            removed = false;
            return false;
        }

        current->documentIds.erase(documentId);
        current->documentFreq = current->documentIds.size();
        removed = true;

        if (current->documentFreq == 0) {
            current->isTerminal = false;
            if (uniqueTerms_ > 0) {
                --uniqueTerms_;
            }
        }

        return !current->isTerminal && current->children.empty() && current != root_.get();
    }

    char c = term[depth];
    auto it = current->children.find(c);
    if (it == current->children.end()) {
        removed = false;
        return false;
    }

    bool shouldPrune = removeTermHelper(it->second.get(), term, depth + 1, documentId, removed);
    if (shouldPrune) {
        current->children.erase(it);
    }

    return !current->isTerminal && current->children.empty() && current != root_.get();
}

void Trie::collectTermsForDoc(const TrieNode* node,
                              string& currentPath,
                              long long documentId,
                              vector<string>& terms) const {
    if (!node) {
        return;
    }

    if (node->isTerminal && node->documentIds.find(documentId) != node->documentIds.end()) {
        terms.push_back(currentPath);
    }

    for (const auto& pair : node->children) {
        currentPath.push_back(pair.first);
        collectTermsForDoc(pair.second.get(), currentPath, documentId, terms);
        currentPath.pop_back();
    }
}

void Trie::removeDocument(long long documentId) {
    vector<string> termsToRemove;
    string path;
    collectTermsForDoc(root_.get(), path, documentId, termsToRemove);

    for (const auto& term : termsToRemove) {
        removeTerm(term, documentId);
    }
}

size_t Trie::uniqueTermCount() const {
    return uniqueTerms_;
}
