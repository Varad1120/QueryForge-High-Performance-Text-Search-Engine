# QueryForge — Low-Level Design (LLD)

**Project:** QueryForge  
**Document status:** Class interfaces, function signatures, data flow contracts, and folder ownership

---

## 1. Folder Ownership by Member

| Folder / Files | Owner | Branch |
|---|---|---|
| `include/structures/trie.hpp`, `src/structures/trie.cpp`, `tests/trie_tests.cpp` | **Tejas** | `dev_tejas` |
| `include/engine/normalizer.hpp`, `src/engine/normalizer.cpp` | **Tejas** | `dev_tejas` |
| `include/structures/suffix_array.hpp`, `src/structures/suffix_array.cpp`, `tests/suffix_array_tests.cpp` | **Kanak** | `dev_kanak` |
| `include/engine/search_engine.hpp`, `src/engine/search_engine.cpp` | **Kanak** | `dev_kanak` |
| `tests/integration_tests.cpp`, `benchmarks/benchmark_main.cpp` | **Kanak** | `dev_kanak` |
| `include/structures/avl_tree.hpp`, `src/structures/avl_tree.cpp`, `tests/avl_tree_tests.cpp` | **Varad** | `dev_varad` |
| `include/structures/max_heap.hpp`, `src/structures/max_heap.cpp`, `tests/max_heap_tests.cpp` | **Varad** | `dev_varad` |
| `include/repository/document_repository.hpp`, `src/repository/document_repository.cpp` | **Varad** | `dev_varad` |
| `sql/schema.sql`, `sql/seed.sql` | **Varad** | `dev_varad` |
| `include/models/`, `src/main.cpp`, `CMakeLists.txt` | **Tejas** (project lead) | `dev_tejas` |
| `docs/`, `README.md`, `.gitignore` | **Tejas** (project lead) | `main` |

---

## 2. Shared Data Models (include/models/)

These structs are the contract between all modules. **No member may change these without notifying the full team.**

```cpp
// include/models/document.hpp
#pragma once
#include <string>
#include <chrono>

struct Document {
    long long  id;
    std::string title;
    std::string body;
    std::string category;
    std::string source;           // optional, may be empty
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point updatedAt;
};
```

```cpp
// include/models/search_result.hpp
#pragma once
#include <string>

struct SearchResult {
    long long   documentId;
    std::string title;
    std::string snippet;          // 150–200 char excerpt around first match
    double      score;
    int         matchCount;
};
```

```cpp
// include/models/search_history_entry.hpp
#pragma once
#include <string>
#include <chrono>

struct SearchHistoryEntry {
    std::string queryText;
    std::string queryType;        // "substring" | "autocomplete" | "range"
    int         requestedLimit;
    int         resultCount;
    long long   elapsedMicros;
};
```

---

## 3. Normalizer (Tejas)

**Files:** `include/engine/normalizer.hpp`, `src/engine/normalizer.cpp`

### Interface

```cpp
// include/engine/normalizer.hpp
#pragma once
#include <string>
#include <vector>

class Normalizer {
public:
    // Convert text to lowercase; strip punctuation; split on whitespace.
    // Returns a list of tokens. Empty input returns an empty vector.
    static std::vector<std::string> tokenize(const std::string& text);

    // Normalize a single string to lowercase; used for query normalization.
    static std::string normalizeToken(const std::string& token);
};
```

### Behavior contract

- All ASCII uppercase letters are converted to lowercase.
- Characters that are not ASCII letters or digits are replaced with a space.
- Multiple consecutive spaces produce a single empty segment that is discarded.
- `tokenize("")` returns `{}`.
- Output tokens are non-empty strings containing only `[a-z0-9]`.

### Edge cases to handle

- All-whitespace input → returns `{}`.
- Input with only punctuation → returns `{}`.
- Numbers are kept as tokens (e.g., `"v2"` → `["v2"]`).

---

## 4. Trie (Tejas)

**Files:** `include/structures/trie.hpp`, `src/structures/trie.cpp`, `tests/trie_tests.cpp`

### Node design

```cpp
struct TrieNode {
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool        isTerminal      = false;
    std::size_t documentFreq    = 0;   // number of distinct docs containing this term
    std::size_t termFreq        = 0;   // total occurrences across all docs (optional)
};
```

### Interface

```cpp
// include/structures/trie.hpp
#pragma once
#include <string>
#include <vector>
#include <memory>

struct TrieSuggestion {
    std::string term;
    std::size_t documentFreq;
};

class Trie {
public:
    Trie();
    ~Trie() = default;

    // Insert a normalized term for a given document.
    // If the term is already present for this document, does not double-count.
    void insertTerm(const std::string& term, long long documentId);

    // Remove a normalized term for a given document.
    // Decrements document frequency. Prunes dead nodes if safe.
    void removeTerm(const std::string& term, long long documentId);

    // Return up to maxSuggestions completions for the given normalized prefix,
    // sorted by documentFreq descending.
    std::vector<TrieSuggestion> autocomplete(
        const std::string& normalizedPrefix,
        std::size_t maxSuggestions = 10) const;

    // Return true if the exact term exists in the Trie.
    bool contains(const std::string& term) const;

    // Remove all entries belonging to a specific document.
    void removeDocument(long long documentId);

    std::size_t uniqueTermCount() const;

private:
    std::unique_ptr<TrieNode> root_;

    TrieNode* findNode(const std::string& prefix) const;
    void collectSuggestions(const TrieNode* node,
                            const std::string& prefix,
                            std::vector<TrieSuggestion>& out) const;
};
```

### Complexity

| Operation | Complexity |
|---|---|
| `insertTerm` | O(L) where L = term length |
| `removeTerm` | O(L) |
| `autocomplete` prefix walk | O(L) |
| Suggestion enumeration | O(V) where V = nodes visited |
| `contains` | O(L) |

### Tests (trie_tests.cpp) — must pass before merging

- Insert and find a single term.
- Prefix with exactly one completion.
- Prefix with multiple completions; check descending frequency order.
- Prefix absent from the Trie returns an empty vector.
- Duplicate insertion from the same document does not increment `documentFreq`.
- Delete a document that shares a prefix with another → remaining term still findable.
- Empty prefix returns completions (capped at `maxSuggestions`).
- `removeDocument` removes all terms belonging to that document.

---

## 5. Suffix Array (Kanak)

**Files:** `include/structures/suffix_array.hpp`, `src/structures/suffix_array.cpp`, `tests/suffix_array_tests.cpp`

### Design choice

One `SuffixArray` object per document. The `DocumentIndex` (in the Search Engine) maintains a `std::unordered_map<long long, SuffixArray>` indexed by `documentId`.

### Interface

```cpp
// include/structures/suffix_array.hpp
#pragma once
#include <string>
#include <vector>

class SuffixArray {
public:
    // Build the suffix array from the given normalized text.
    explicit SuffixArray(const std::string& normalizedText);

    // Return all starting offsets in the text where `pattern` occurs.
    // Returns an empty vector if not found or if text/pattern is empty.
    std::vector<int> search(const std::string& pattern) const;

    // Number of suffixes (= text length).
    std::size_t size() const;

    // The normalized text this array was built from.
    const std::string& text() const;

private:
    std::string        text_;
    std::vector<int>   sa_;    // suffix array: sa_[i] = start of i-th sorted suffix

    void buildPrefixDoubling();

    // Compare suffix at position sa_[lo..hi] against pattern for binary search.
    int compareAtPosition(int pos, const std::string& pattern) const;
};
```

### Build algorithm (prefix doubling)

1. Initialize `sa_` as `[0, 1, 2, ..., N-1]`.
2. Sort by first character; assign ranks.
3. Double the comparison window each pass: sort by (rank[i], rank[i + gap]) pairs.
4. Re-rank; stop when all ranks are unique.
5. Final sort is by canonical suffix order.

This achieves O(N log N) in practice under typical text distributions.

### Pattern search (binary search)

- Binary search for the first occurrence (lower bound) using the first `P` characters of each suffix.
- Binary search for the last occurrence (upper bound).
- Collect all offsets `sa_[lo]` through `sa_[hi]`.

### Tests (suffix_array_tests.cpp)

- Pattern found exactly once; correct offset returned.
- Pattern found multiple times; all offsets returned.
- Pattern not present → empty result.
- Pattern longer than text → empty result.
- Repeated characters (`"aaaaaa"`, pattern `"aa"`) → all offsets.
- Empty text → gracefully returns empty.
- Pattern at start of text (offset 0).
- Pattern at end of text.
- Differential test: compare results to `std::string::find` scan over 50 random strings.

---

## 6. AVL Tree (Varad)

**Files:** `include/structures/avl_tree.hpp`, `src/structures/avl_tree.cpp`, `tests/avl_tree_tests.cpp`

### Node design

```cpp
struct AVLNode {
    long long key;
    int       height;
    AVLNode*  left;
    AVLNode*  right;

    explicit AVLNode(long long k)
        : key(k), height(1), left(nullptr), right(nullptr) {}
};
```

### Interface

```cpp
// include/structures/avl_tree.hpp
#pragma once
#include <vector>

class AVLTree {
public:
    AVLTree();
    ~AVLTree();

    void   insert(long long key);
    void   remove(long long key);
    bool   contains(long long key) const;

    // Return all keys in [low, high] in ascending order.
    std::vector<long long> rangeQuery(long long low, long long high) const;

    // Return all keys in ascending order.
    std::vector<long long> inOrder() const;

    // For validation only — returns height of root (0 if empty).
    int height() const;

    // Debug: verify BST property and balance factors for all nodes.
    bool isValid() const;

private:
    AVLNode* root_;

    // Core helpers
    int  nodeHeight(const AVLNode* n) const;
    int  balanceFactor(const AVLNode* n) const;
    void updateHeight(AVLNode* n);

    // Rotations
    AVLNode* rotateRight(AVLNode* y);
    AVLNode* rotateLeft(AVLNode* x);

    // Rebalance after insert/delete
    AVLNode* rebalance(AVLNode* n);

    // Recursive helpers
    AVLNode* insertRec(AVLNode* n, long long key);
    AVLNode* removeRec(AVLNode* n, long long key);
    AVLNode* minNode(AVLNode* n) const;

    // Traversal helpers
    void inOrderRec(const AVLNode* n, std::vector<long long>& out) const;
    void rangeRec(const AVLNode* n, long long lo, long long hi,
                  std::vector<long long>& out) const;

    // Destructor helper
    void destroyRec(AVLNode* n);

    // Validation helper
    bool isValidRec(const AVLNode* n, long long* minKey, long long* maxKey) const;
};
```

### Rotation cases

| Imbalance | Case | Fix |
|---|---|---|
| Left-Left (LL) | Left subtree too tall, inserted left | Right rotation |
| Right-Right (RR) | Right subtree too tall, inserted right | Left rotation |
| Left-Right (LR) | Left subtree too tall, inserted right | Left rotate left child, then right rotate root |
| Right-Left (RL) | Right subtree too tall, inserted left | Right rotate right child, then left rotate root |

Balance factor = `height(left) − height(right)`. Must satisfy `|bf| ≤ 1` at every node.

### Tests (avl_tree_tests.cpp)

- Insert sorted keys 1..20; verify `isValid()` passes.
- Insert reverse-sorted keys 20..1; verify `isValid()` passes.
- Random insertion order; verify `isValid()` after each.
- Delete a leaf node; verify `isValid()`.
- Delete a node with one child; verify `isValid()`.
- Delete a node with two children (in-order successor); verify `isValid()`.
- `contains()` on present and absent keys.
- `rangeQuery(5, 15)` returns correct sorted slice.
- `inOrder()` returns sorted sequence.
- Insert duplicate key — define behavior (ignore or allow); test it.

---

## 7. Max-Heap (Varad)

**Files:** `include/structures/max_heap.hpp`, `src/structures/max_heap.cpp`, `tests/max_heap_tests.cpp`

### Interface

```cpp
// include/structures/max_heap.hpp
#pragma once
#include "models/search_result.hpp"
#include <vector>

// Comparator: higher score is "greater"; on tie, lower documentId is "greater" (deterministic).
struct SearchResultComparator {
    bool operator()(const SearchResult& a, const SearchResult& b) const;
};

class MaxHeap {
public:
    MaxHeap();
    explicit MaxHeap(std::vector<SearchResult> candidates); // builds in O(M)

    void push(const SearchResult& result);
    SearchResult pop();       // removes and returns the highest-scoring element
    const SearchResult& top() const;

    bool   empty() const;
    std::size_t size() const;

    // Extract the top K results in descending score order.
    // Leaves the heap empty afterwards.
    std::vector<SearchResult> extractTopK(std::size_t k);

private:
    std::vector<SearchResult> data_;

    void heapifyUp(std::size_t idx);
    void heapifyDown(std::size_t idx);

    std::size_t parent(std::size_t i) const { return (i - 1) / 2; }
    std::size_t leftChild(std::size_t i) const { return 2 * i + 1; }
    std::size_t rightChild(std::size_t i) const { return 2 * i + 2; }
};
```

### Relevance score formula

```
score = 5.0 * title_match
      + 1.0 * log(1 + body_occurrence_count)
      + 0.5 * recency_factor
```

- `title_match` = 1 if normalized query appears in normalized title, else 0.
- `body_occurrence_count` = number of literal occurrences found by Suffix Array.
- `recency_factor` = `max(0.0, 1.0 − days_since_creation / 365.0)`, clamped to [0, 1].
- Coefficients are constants defined in `search_engine.hpp` and must be documented in the report.

### Tests (max_heap_tests.cpp)

- Push candidates with increasing scores; `extractTopK(3)` returns top 3 in descending order.
- Push candidates with decreasing scores; same expectation.
- Equal scores: tie broken by ascending `documentId`.
- `extractTopK(K)` when fewer than K candidates exist: returns all in order.
- `extractTopK(0)` returns empty vector.
- Empty heap: `empty()` is true; `pop()` on empty heap throws or is guarded.
- Build from vector constructor: O(M) path; same ordering as incremental pushes.
- Differential test: compare `extractTopK(K)` against sorted vector for 30 random inputs.

---

## 8. Repository Layer (Varad)

**Files:** `include/repository/document_repository.hpp`, `src/repository/document_repository.cpp`

### Interface

```cpp
// include/repository/document_repository.hpp
#pragma once
#include "models/document.hpp"
#include "models/search_result.hpp"
#include "models/search_history_entry.hpp"
#include <vector>
#include <string>
#include <pqxx/pqxx>

struct BenchmarkRun {
    std::string datasetName;
    long long   datasetSize;
    std::string algorithmName;
    std::string operationName;
};

struct BenchmarkMeasurement {
    long long benchmarkRunId;
    int       iterationNumber;
    long long elapsedMicros;
    long long resultCount;
    long long memoryBytes;   // optional, 0 if not measured
};

class DocumentRepository {
public:
    explicit DocumentRepository(const std::string& connectionString);

    // Document CRUD
    std::vector<Document> getAllDocuments();
    Document              getDocumentById(long long id);
    long long             insertDocument(const Document& doc);
    void                  updateDocument(const Document& doc);
    void                  deleteDocument(long long id);

    // Term statistics (for persistent Trie backing)
    void upsertTermStats(const std::string& normalizedTerm,
                         long long documentId,
                         int termFrequency);
    void deleteTermStats(long long documentId);

    // Search history
    void insertSearchHistory(const SearchHistoryEntry& entry);
    std::vector<SearchHistoryEntry> getRecentSearches(int limit = 50);

    // Benchmarks
    long long insertBenchmarkRun(const BenchmarkRun& run);
    void      insertBenchmarkMeasurement(const BenchmarkMeasurement& m);

private:
    pqxx::connection conn_;

    void executeNonQuery(const std::string& sql,
                         const std::vector<std::string>& params = {});
};
```

### Key design rules

- All queries use pqxx parameterized statements (`pqxx::work::exec_params`).
- Document insert/update also updates `terms` and `document_terms` in the same transaction.
- `getAllDocuments()` is the only source used during index reconstruction at startup.
- `deleteDocument` relies on `ON DELETE CASCADE` in the schema to clean `document_terms`.

---

## 9. Search Engine (Kanak)

**Files:** `include/engine/search_engine.hpp`, `src/engine/search_engine.cpp`

### Interface

```cpp
// include/engine/search_engine.hpp
#pragma once
#include "structures/trie.hpp"
#include "structures/suffix_array.hpp"
#include "structures/avl_tree.hpp"
#include "structures/max_heap.hpp"
#include "repository/document_repository.hpp"
#include "models/document.hpp"
#include "models/search_result.hpp"
#include <vector>
#include <string>
#include <unordered_map>

enum class IndexState { NOT_BUILT, READY, STALE };

class SearchEngine {
public:
    explicit SearchEngine(DocumentRepository& repo);

    // Load all documents from DB and build all indexes.
    void buildIndexes();

    // Autocomplete: return up to maxSuggestions terms starting with prefix.
    std::vector<TrieSuggestion> autocomplete(
        const std::string& prefix,
        std::size_t maxSuggestions = 10);

    // Substring search: return top K ranked results.
    std::vector<SearchResult> search(
        const std::string& query,
        int limit = 10);

    // AVL range query: return document IDs in [fromId, toId].
    std::vector<long long> rangeQuery(long long fromId, long long toId);

    // CRUD — updates DB and then incrementally updates indexes.
    long long addDocument(Document doc);
    void      updateDocument(Document doc);
    void      deleteDocument(long long id);

    IndexState indexState() const { return state_; }
    std::size_t documentCount() const;

private:
    DocumentRepository& repo_;

    Trie                                         trie_;
    std::unordered_map<long long, SuffixArray>   suffixArrays_;
    AVLTree                                      avlTree_;
    IndexState                                   state_ = IndexState::NOT_BUILT;

    // Held in memory for scoring (recency factor)
    std::unordered_map<long long, Document>      documentCache_;

    void indexDocument(const Document& doc);
    void deindexDocument(long long id);

    double computeScore(long long docId,
                        const std::string& normalizedQuery,
                        int matchCount) const;
    std::string extractSnippet(const std::string& body,
                               const std::string& query,
                               int maxLength = 180) const;
};
```

### Build-indexes flow

1. `repo_.getAllDocuments()` — load all documents.
2. For each document: tokenize body and title, insert all tokens into Trie, build a `SuffixArray` from the normalized body, insert `document_id` into AVL Tree, cache document in `documentCache_`.
3. Set `state_ = IndexState::READY`.

### Search flow

1. Normalize `query`.
2. For each `(id, sa)` in `suffixArrays_`: call `sa.search(normalizedQuery)` → count matches.
3. For documents with `matchCount > 0`: compute `score`, build `SearchResult`.
4. Build `MaxHeap` from all candidates.
5. `extractTopK(limit)`.
6. For each result: fetch snippet, fill title from `documentCache_`.
7. `repo_.insertSearchHistory(...)`.
8. Return result list.

---

## 10. Main Entry Point (Tejas)

**File:** `src/main.cpp`

`main.cpp` is responsible for:

1. Reading the PostgreSQL connection string from an environment variable (`QUERYFORGE_DB_URL`) or a config file.
2. Constructing `DocumentRepository` and `SearchEngine`.
3. Calling `searchEngine.buildIndexes()` at startup with timing output.
4. Parsing CLI arguments and dispatching to the appropriate `SearchEngine` or `DocumentRepository` method.
5. Handling unknown commands and printing a help message.

Example startup output:
```
[INFO] Connected to PostgreSQL.
[INFO] Loaded 1842 documents.
[INFO] Trie built. Unique terms: 14,209.
[INFO] Suffix arrays built. Total text indexed: 3.2 MB.
[INFO] AVL tree built. Height: 11.
[INFO] Indexes ready. Build time: 0.41 s.
QueryForge> 
```

---

## 11. Interface Boundaries (Independence Contract)

Each member's module compiles and passes unit tests without the other members' code. This is achieved as follows:

| Boundary | How independence is maintained |
|---|---|
| Trie & Normalizer (Tejas) | No database dependency; takes `std::string` input; tested standalone |
| Suffix Array (Kanak) | Takes `std::string` in constructor; no DB or Trie dependency |
| AVL Tree & Max-Heap (Varad) | Takes `long long` keys or `SearchResult` structs; no DB or Trie |
| Repository (Varad) | Depends only on shared models and libpqxx; not on other structures |
| Search Engine (Kanak) | Owns `Trie`, `SuffixArray` map, `AVLTree` — depends on all; integrated last |

**Merge order:** Tejas merges first (normalizer + Trie + models + CMakeLists). Varad and Kanak merge independently after that. Search Engine is integrated in the final merge checkpoint.
