# QueryForge — High-Level Design (HLD)

**Project:** QueryForge — High-Performance Text Search Engine Using Advanced Data Structures  
**Type:** ADS (Advanced Data Structures) Mini-Project  
**Language:** C++17 or C++20  
**Database:** PostgreSQL  
**Team:** Tejas · Kanak · Varad  
**Document status:** High-level architecture and component overview

---

## 1. Executive Summary

QueryForge is a text-search engine for a local collection of plain-text documents such as lecture notes, technical articles, and research abstracts. It supports document management (add, import, view, update, delete), literal substring search, prefix autocomplete, top-K ranked results, ordered metadata queries, search-history reporting, and reproducible benchmark comparisons.

The academic goal of the project is not just to build a working search tool but to demonstrate that four specific advanced data structures are implemented in C++, own a defined computational responsibility, and are tested independently against reference baselines.

The four core data structures are:

| Structure | Responsibility |
|---|---|
| **Trie** | Prefix autocomplete over normalized vocabulary |
| **Suffix Array** | Literal substring search over indexed document text |
| **AVL Tree** | Ordered lookup and range queries over document metadata |
| **Max-Heap** | Top-K ranked result selection |

A PostgreSQL database stores canonical documents, persistent term statistics, search history, and benchmark measurements. The C++ engine builds all in-memory indexes from the database at startup and updates them incrementally on mutations.

---

## 2. Goals and Non-Goals

### Functional goals

- Store and retrieve documents persistently via PostgreSQL.
- Import a directory of `.txt` files.
- Normalize text consistently for indexing and querying.
- Offer prefix autocomplete using a Trie.
- Search for literal substrings or phrases using per-document Suffix Arrays.
- Rank matching documents and return the top K results using a Max-Heap.
- Perform ordered metadata range queries using an AVL Tree.
- Insert, update, and delete documents while keeping indexes consistent.
- Persist search history and benchmark measurements.
- Provide a CLI-first interface; a web UI is an optional extension.

### Academic goals

- Implement all four structures in C++ without substituting standard-library containers.
- Explain each structure's invariants and operations in the final report.
- Benchmark each structure against a documented baseline.
- Test correctness differentially against simple reference implementations.

### Non-goals for the first version

- Replacing a production search engine (Elasticsearch, Solr, etc.).
- Distributed indexing or multi-node search.
- Unicode-aware tokenization beyond ASCII normalization.
- Semantic or embedding-based retrieval.
- Full-document-format parsing for PDF or DOCX.
- Guaranteed sub-millisecond latency.
- Implementing a full inverted-index engine (unless added as an extension).

---

## 3. System Architecture

### 3.1 High-level architecture diagram

```
                    +---------------------------------+
                    |         User / CLI              |
                    |  search, add, import, benchmark |
                    +----------------+----------------+
                                     |
                                     v
                    +---------------------------------+
                    |      Application Layer          |
                    |  command parsing, validation,   |
                    |  error reporting, DTOs          |
                    +----------------+----------------+
                                     |
                                     v
                    +---------------------------------+
                    |       Search Engine             |
                    |  normalizer, query dispatch,    |
                    |  match collection, scoring      |
                    +------+-------+-------+----------+
                           |       |       |
                     Trie  | Sfx.  | Max-  | AVL
                           | Array | Heap  | Tree
                    +------+ +-----+ +-----+ +------+
                    | Trie | | SA  | | Heap| | AVL  |
                    +------+ +-----+ +-----+ +------+
                                     |
                                     v
                    +---------------------------------+
                    |      Repository Layer           |
                    |  SQL queries, row mapping,      |
                    |  transaction management         |
                    +----------------+----------------+
                                     |
                                     v
                    +---------------------------------+
                    |        PostgreSQL               |
                    |  documents, terms, history,     |
                    |  benchmark runs & measurements  |
                    +---------------------------------+
```

### 3.2 Layer responsibilities

| Layer | Responsibility |
|---|---|
| CLI | Accept commands, display suggestions, results, and stats |
| Application layer | Parse and validate commands, coordinate workflows, define DTOs |
| Search engine | Normalize queries, dispatch to the right index, merge matches, score and assemble results |
| Data-structure layer | Trie, Suffix Array, AVL Tree, and Max-Heap — no SQL dependency |
| Repository layer | Encapsulate all SQL access; map rows to C++ structs |
| Database | Persist canonical documents, term statistics, search logs, and benchmark data |
| Benchmark layer | Run repeatable timed experiments and persist measurements |

Each data structure is testable without a running database. The repository layer supplies records; the structures index them.

---

## 4. Module Descriptions

### 4.1 Normalizer

Converts raw text to a canonical form used for both indexing and querying. Responsibilities:

- Lowercase all ASCII letters.
- Discard punctuation and treat it as a word separator.
- Tokenize on whitespace and punctuation boundaries.
- Strip leading and trailing whitespace from each token.
- Return an empty token list for empty or whitespace-only input.

Normalization must be identical at index time and at query time, or searches will fail silently. Centralize this in one `Normalizer` class or header.

### 4.2 Trie

Stores the normalized vocabulary across all indexed documents. Supports:

- Insert a term (O(L) with hash-map children).
- Delete a term (O(L), with decrement of document frequency).
- Prefix autocomplete: find the prefix node, then enumerate completions ranked by document frequency.

Each terminal node records `documentFrequency` (number of distinct documents containing the term). On document deletion, decrement affected nodes and prune nodes that no longer belong to any term.

### 4.3 Suffix Array

Maintains a sorted array of suffix starting positions for each document's normalized text. Supports:

- Build from text using the prefix-doubling algorithm (O(N log N)).
- Binary search for a query pattern (O(P log N)).
- Return all matching offsets within the document.

One suffix array per document is the recommended initial design. Querying is distributed across all document arrays and results are merged into a candidate set.

### 4.4 AVL Tree

A self-balancing binary search tree keyed on document ID. Supports:

- Insert (O(log N)).
- Delete (O(log N)).
- Search by key (O(log N)).
- In-order traversal (O(N)).
- Range query for IDs in [A, B] (O(log N + R)).

The AVL Tree is the application's ordered metadata index, not the same as PostgreSQL's internal indexes. Rotations (LL, RR, LR, RL) maintain the balance invariant |height(left) − height(right)| ≤ 1.

### 4.5 Max-Heap

Holds `SearchResult` candidates keyed on relevance score. Supports:

- Build from a vector of candidates in O(M) time.
- Extract the top K results in O(K log M).
- Deterministic tie-breaking by ascending document ID when scores are equal.

A bounded min-heap of size K may be used as an alternative for streaming top-K selection; document which approach was chosen in the report.

### 4.6 Search Engine

The central coordinator. Responsibilities:

- Normalize the incoming query.
- For autocomplete: query the Trie and return ranked suggestions.
- For substring search: query per-document Suffix Arrays, deduplicate and count matches, compute relevance scores, insert candidates into the Max-Heap, extract top K.
- For range queries: query the AVL Tree.
- Persist search metadata to the database after every search.

### 4.7 Repository Layer

Wraps all SQL interaction. Exposes methods such as:

- `getAllDocuments()` — used during startup to build indexes.
- `insertDocument(doc)` / `updateDocument(doc)` / `deleteDocument(id)`.
- `insertSearchHistory(entry)`.
- `insertBenchmarkRun(run)` / `insertBenchmarkMeasurement(m)`.

Uses parameterized queries (libpqxx) to prevent SQL injection. Manages transactions for document mutations that also update the `terms` and `document_terms` tables.

### 4.8 CLI

Provides commands:

```
queryforge import ./documents/
queryforge add
queryforge show <id>
queryforge update <id>
queryforge delete <id>
queryforge autocomplete "<prefix>"
queryforge search "<query>" [--limit K]
queryforge range <from_id> <to_id>
queryforge rebuild-indexes
queryforge benchmark [--dataset small|medium|large]
queryforge stats
```

### 4.9 Benchmark Runner

Runs timed experiments comparing custom structures against simple baselines:

| Feature | Custom | Baseline |
|---|---|---|
| Prefix autocomplete | Trie | Linear vocabulary scan |
| Substring search | Suffix Array | Naive `std::string::find` scan |
| Ordered key lookup | AVL Tree | Linear search in a `std::vector` |
| Top-K ranking | Max-Heap | `std::sort` all candidates, take first K |

Reports median, p95, and average over repeated runs. Persists results to `benchmark_runs` and `benchmark_measurements`.

---

## 5. Data Flow

### 5.1 Startup and index construction

```
Connect to PostgreSQL
        |
Read all documents from DB
        |
Normalize each document body and title
        |
Build Trie from all tokens
        |
Build per-document Suffix Array
        |
Build AVL Tree from document IDs
        |
Mark indexes READY
        |
Accept CLI commands
```

### 5.2 Autocomplete query

```
User inputs prefix string
        |
Normalize prefix
        |
Trie: walk to prefix node → enumerate completions
        |
Rank suggestions by document frequency
        |
Return top N suggestions to user
```

### 5.3 Substring search query

```
User inputs query and optional limit K
        |
Normalize query
        |
For each document: binary-search Suffix Array
        |
Collect (document_id, match_count) pairs
        |
Compute relevance score per document
        |
Build Max-Heap from scored candidates
        |
Extract top K results
        |
Persist search entry to search_history
        |
Display results with title, snippet, match count
```

### 5.4 Insert/Update/Delete flow

```
Validate input
        |
Begin DB transaction
        |
Persist document change to PostgreSQL
        |
Commit transaction
        |
Update Trie, Suffix Array, AVL Tree incrementally
        |
If update fails → mark indexes STALE, do not serve indexed search
        |
On next startup → rebuild from DB
```

---

## 6. Technology Stack

| Component | Choice |
|---|---|
| Language | C++17 or C++20 |
| Build system | CMake (3.20+) |
| Database | PostgreSQL |
| DB client library | libpqxx |
| Testing framework | Catch2 or GoogleTest |
| Version control | Git, one branch per member |
| Optional frontend | React + Vite (stretch goal) |

---

## 7. Repository Structure

```
queryforge/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── HLD.md
│   ├── LLD.md
│   ├── DATABASE_DESIGN.md
│   └── TEAM_PLAN.md
├── include/
│   ├── models/
│   │   ├── document.hpp
│   │   └── search_result.hpp
│   ├── structures/
│   │   ├── trie.hpp
│   │   ├── suffix_array.hpp
│   │   ├── avl_tree.hpp
│   │   └── max_heap.hpp
│   ├── engine/
│   │   ├── search_engine.hpp
│   │   └── normalizer.hpp
│   └── repository/
│       └── document_repository.hpp
├── src/
│   ├── main.cpp
│   ├── structures/
│   │   ├── trie.cpp
│   │   ├── suffix_array.cpp
│   │   ├── avl_tree.cpp
│   │   └── max_heap.cpp
│   ├── engine/
│   │   ├── search_engine.cpp
│   │   └── normalizer.cpp
│   └── repository/
│       └── document_repository.cpp
├── sql/
│   ├── schema.sql
│   └── seed.sql
├── tests/
│   ├── trie_tests.cpp
│   ├── suffix_array_tests.cpp
│   ├── avl_tree_tests.cpp
│   ├── max_heap_tests.cpp
│   └── integration_tests.cpp
├── benchmarks/
│   ├── benchmark_main.cpp
│   └── datasets/
│       ├── small/
│       ├── medium/
│       └── large/
└── data/
    └── sample_documents/
```

---

## 8. Security and Reliability Considerations

- Use parameterized SQL queries; never concatenate user input into SQL strings.
- Validate document IDs, result limits, and query lengths before processing.
- Cap maximum query length (e.g., 1000 characters) and autocomplete output (e.g., 20 suggestions).
- Handle database disconnections and failed transactions with clear error messages.
- Use RAII and `std::unique_ptr` to prevent memory leaks in tree and trie nodes.
- Ensure destructors are correct for all pointer-based data structures.
- Avoid undefined behavior in comparators (strict weak ordering) and integer arithmetic.
- Keep benchmark data and production document data in separate dataset directories.
- Do not log full document bodies; log IDs and truncated titles only.

---

## 9. Risk Summary

| Risk | Impact | Mitigation |
|---|---|---|
| Suffix Array build time is slow for large text | Startup latency | Implement prefix-doubling correctly; benchmark build vs query separately |
| Inconsistent normalization between index and query | Silent search failures | Centralize normalization in one function; unit-test it |
| AVL rotation bugs | Corrupted ordered queries | Validate height and balance invariants after every mutation in tests |
| Memory growth from suffix indexes | OOM for large datasets | Measure memory; cap dataset size in benchmarks; use per-document arrays |
| Database and in-memory index drift | Stale search results | Rebuild from DB on startup; mark stale on failed update |
| Over-scoping with UI | Core structures incomplete | Deliver a CLI MVP first; add UI only if core is stable |

---

## 10. Minimum Viable Product

The MVP must include all of the following before any optional extension is considered:

- C++ implementations of Trie, Suffix Array, AVL Tree, and Max-Heap.
- PostgreSQL-backed document import and CRUD.
- Index construction from stored documents at startup.
- Working prefix autocomplete.
- Working literal substring search.
- Top-K ranked results.
- Ordered AVL range query (document ID range).
- Search history persistence.
- Baseline comparisons and a written benchmark report.
- Unit tests for each structure and integration tests for end-to-end flows.
- Setup instructions and documented limitations.
