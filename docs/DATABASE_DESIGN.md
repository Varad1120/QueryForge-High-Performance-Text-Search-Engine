# QueryForge — Database Design

**Project:** QueryForge  
**Database:** PostgreSQL  
**Owner:** Varad (`dev_varad`)  
**Document status:** Full schema, normalization analysis, ER overview, and sample queries

---

## 1. Design Principles

- The **database is the source of truth**. All in-memory C++ indexes are derived and can be rebuilt from it.
- Every entity has a surrogate primary key (`BIGINT GENERATED ALWAYS AS IDENTITY`).
- Foreign keys enforce referential integrity; cascaded deletes keep dependent rows consistent.
- Transactions wrap document mutations that also touch `terms` and `document_terms`.
- Parameterized queries are mandatory at the application layer to prevent SQL injection.
- The database does **not** perform full-text search or autocomplete — that is the job of the C++ structures. Standard PostgreSQL indexes are used only for ordinary row lookups.

---

## 2. Changes from the Proposal Schema

The following adjustments were made relative to the project proposal:

| Change | Reason |
|---|---|
| Added `source TEXT` column to `documents` | Support bulk import attribution (directory name or URL) |
| Added `query_type VARCHAR(30)` to `search_history` | Distinguish autocomplete, substring, and range queries in reporting |
| `JSONB configuration` in `benchmark_runs` | Stores K value, dataset seed, compiler flags — avoids extra columns |
| `memory_bytes BIGINT` in `benchmark_measurements` | Enables memory trade-off analysis alongside latency |
| Separate `terms` and `document_terms` tables retained | Provides persistent term statistics for Trie initialization and reporting, independent of the runtime Trie |

---

## 3. Entity Relationship Overview

```
documents (1) ──────── (N) document_terms (N) ──── (1) terms
     │
     └──────── (N) search_result_snapshots   ←──── (1) search_history
                                                         │
                                          benchmark_runs (1) ──── (N) benchmark_measurements
```

Minimum required tables (MVP): `documents`, `search_history`, `benchmark_runs`, `benchmark_measurements`.

Optional tables: `terms`, `document_terms`, `search_result_snapshots` (for persistent result logging).

---

## 4. Full Schema (schema.sql)

```sql
-- ============================================================
-- QueryForge PostgreSQL Schema
-- Run with: psql -U <user> -d <dbname> -f sql/schema.sql
-- ============================================================

-- Drop order respects foreign keys
DROP TABLE IF EXISTS benchmark_measurements CASCADE;
DROP TABLE IF EXISTS benchmark_runs CASCADE;
DROP TABLE IF EXISTS search_result_snapshots CASCADE;
DROP TABLE IF EXISTS search_history CASCADE;
DROP TABLE IF EXISTS document_terms CASCADE;
DROP TABLE IF EXISTS terms CASCADE;
DROP TABLE IF EXISTS documents CASCADE;

-- ============================================================
-- documents: canonical source of truth for all indexed text
-- ============================================================
CREATE TABLE documents (
    document_id  BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    title        TEXT NOT NULL,
    body         TEXT NOT NULL,
    category     VARCHAR(100),
    source       TEXT,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at   TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_documents_category    ON documents (category);
CREATE INDEX idx_documents_created_at  ON documents (created_at DESC);

-- ============================================================
-- terms: persistent normalized vocabulary
-- document_frequency = number of distinct documents containing this term
-- ============================================================
CREATE TABLE terms (
    term_id            BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    normalized_term    TEXT NOT NULL UNIQUE,
    document_frequency BIGINT NOT NULL DEFAULT 0
        CONSTRAINT chk_doc_freq_non_negative CHECK (document_frequency >= 0)
);

-- ============================================================
-- document_terms: per-document term occurrence statistics
-- ============================================================
CREATE TABLE document_terms (
    document_id    BIGINT NOT NULL
        REFERENCES documents (document_id) ON DELETE CASCADE,
    term_id        BIGINT NOT NULL
        REFERENCES terms (term_id) ON DELETE CASCADE,
    term_frequency INTEGER NOT NULL
        CONSTRAINT chk_term_freq_positive CHECK (term_frequency > 0),
    PRIMARY KEY (document_id, term_id)
);

CREATE INDEX idx_document_terms_term_id ON document_terms (term_id);

-- ============================================================
-- search_history: every search call is logged
-- ============================================================
CREATE TABLE search_history (
    search_id       BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    query_text      TEXT NOT NULL,
    query_type      VARCHAR(30) NOT NULL
        CONSTRAINT chk_query_type
            CHECK (query_type IN ('substring', 'autocomplete', 'range')),
    requested_limit INTEGER NOT NULL DEFAULT 10
        CONSTRAINT chk_limit_positive CHECK (requested_limit > 0),
    result_count    INTEGER NOT NULL DEFAULT 0
        CONSTRAINT chk_result_non_negative CHECK (result_count >= 0),
    elapsed_us      BIGINT
        CONSTRAINT chk_elapsed_non_negative CHECK (elapsed_us >= 0),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- search_result_snapshots: optional — persist top results per search
-- ============================================================
CREATE TABLE search_result_snapshots (
    snapshot_id  BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    search_id    BIGINT NOT NULL
        REFERENCES search_history (search_id) ON DELETE CASCADE,
    document_id  BIGINT NOT NULL
        REFERENCES documents (document_id) ON DELETE CASCADE,
    rank_position INTEGER NOT NULL CONSTRAINT chk_rank_pos CHECK (rank_position >= 1),
    score        DOUBLE PRECISION NOT NULL,
    match_count  INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX idx_snapshots_search_id ON search_result_snapshots (search_id);

-- ============================================================
-- benchmark_runs: one row per experiment setup
-- configuration JSONB stores: dataset_seed, k_value, compiler_flags, etc.
-- ============================================================
CREATE TABLE benchmark_runs (
    benchmark_run_id  BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    dataset_name      TEXT NOT NULL,
    dataset_size      BIGINT NOT NULL
        CONSTRAINT chk_dataset_size_non_negative CHECK (dataset_size >= 0),
    algorithm_name    VARCHAR(100) NOT NULL,
    operation_name    VARCHAR(100) NOT NULL,
    configuration     JSONB,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP
);

-- ============================================================
-- benchmark_measurements: one row per iteration of a run
-- ============================================================
CREATE TABLE benchmark_measurements (
    measurement_id    BIGINT GENERATED ALWAYS AS IDENTITY PRIMARY KEY,
    benchmark_run_id  BIGINT NOT NULL
        REFERENCES benchmark_runs (benchmark_run_id) ON DELETE CASCADE,
    iteration_number  INTEGER NOT NULL
        CONSTRAINT chk_iter_positive CHECK (iteration_number > 0),
    elapsed_us        BIGINT NOT NULL
        CONSTRAINT chk_meas_elapsed CHECK (elapsed_us >= 0),
    result_count      BIGINT,
    memory_bytes      BIGINT
        CONSTRAINT chk_mem_non_negative CHECK (memory_bytes >= 0)
);

CREATE INDEX idx_measurements_run_id ON benchmark_measurements (benchmark_run_id);
```

---

## 5. Seed Data (seed.sql)

```sql
-- ============================================================
-- QueryForge Seed Data
-- Run after schema.sql
-- ============================================================

INSERT INTO documents (title, body, category, source) VALUES
('Introduction to Algorithms',
 'An algorithm is a step-by-step procedure for solving a problem. Common algorithms include sorting and searching.',
 'algorithms', 'sample_documents'),

('Binary Search Trees',
 'A binary search tree stores keys in sorted order. Each node has at most two children. Search, insert and delete all run in O(log n) average time.',
 'data-structures', 'sample_documents'),

('AVL Trees and Balancing',
 'An AVL tree is a self-balancing binary search tree. The balance factor of every node is maintained between -1 and 1 using rotations.',
 'data-structures', 'sample_documents'),

('Suffix Arrays for Pattern Matching',
 'A suffix array contains sorted suffixes of a string. Binary search over the array enables pattern matching in O(P log N) time.',
 'algorithms', 'sample_documents'),

('Trie Data Structure',
 'A trie is a tree-like structure for storing strings. It supports fast prefix queries and is commonly used for autocomplete systems.',
 'data-structures', 'sample_documents'),

('Heaps and Priority Queues',
 'A max-heap is a complete binary tree where every parent key is greater than its children. It supports extract-max in O(log n) time.',
 'data-structures', 'sample_documents'),

('Database Indexing',
 'Database indexes accelerate queries by maintaining sorted structures over column values. B-trees and hash indexes are common choices.',
 'databases', 'sample_documents'),

('Text Search Engines',
 'A full-text search engine builds inverted indexes mapping terms to documents. Scoring models such as TF-IDF rank results by relevance.',
 'search', 'sample_documents'),

('Graph Algorithms',
 'Graph algorithms traverse edges and vertices to solve problems. Breadth-first search and Dijkstra are widely used for path finding.',
 'algorithms', 'sample_documents'),

('Dynamic Programming',
 'Dynamic programming solves problems by breaking them into overlapping subproblems and caching intermediate results. Memoization is a common technique.',
 'algorithms', 'sample_documents');
```

---

## 6. Table Descriptions

### 6.1 documents

| Column | Type | Notes |
|---|---|---|
| `document_id` | BIGINT (identity) | Primary key; used as AVL Tree key |
| `title` | TEXT | Required; normalized for Trie and scoring |
| `body` | TEXT | Required; normalized text used for Suffix Array construction |
| `category` | VARCHAR(100) | Optional filter dimension |
| `source` | TEXT | Import directory or origin; nullable |
| `created_at` | TIMESTAMPTZ | Set on insert; used for recency scoring |
| `updated_at` | TIMESTAMPTZ | Updated on every `UPDATE`; application must maintain this |

### 6.2 terms

| Column | Type | Notes |
|---|---|---|
| `term_id` | BIGINT (identity) | Primary key |
| `normalized_term` | TEXT | Unique; must be normalized the same way as the C++ Trie |
| `document_frequency` | BIGINT | Count of distinct documents containing this term; maintained by the application in the same transaction as document mutations |

### 6.3 document_terms

| Column | Type | Notes |
|---|---|---|
| `document_id` | BIGINT (FK → documents) | Cascading delete |
| `term_id` | BIGINT (FK → terms) | Cascading delete |
| `term_frequency` | INTEGER | Occurrences of this term in this document |

### 6.4 search_history

| Column | Type | Notes |
|---|---|---|
| `search_id` | BIGINT (identity) | Primary key |
| `query_text` | TEXT | Raw user query |
| `query_type` | VARCHAR(30) | `'substring'`, `'autocomplete'`, or `'range'` |
| `requested_limit` | INTEGER | K value passed to the search call |
| `result_count` | INTEGER | Number of results actually returned |
| `elapsed_us` | BIGINT | Wall-clock time of the C++ search in microseconds |

### 6.5 benchmark_runs

| Column | Type | Notes |
|---|---|---|
| `benchmark_run_id` | BIGINT (identity) | Primary key |
| `dataset_name` | TEXT | `'small'`, `'medium'`, `'large'`, etc. |
| `dataset_size` | BIGINT | Document count used in this run |
| `algorithm_name` | VARCHAR(100) | `'Trie'`, `'SuffixArray'`, `'AVLTree'`, `'MaxHeap'`, `'NaiveScan'`, etc. |
| `operation_name` | VARCHAR(100) | `'autocomplete'`, `'search'`, `'topk'`, `'range'`, etc. |
| `configuration` | JSONB | `{"k": 10, "warmup_runs": 3, "compiler": "g++ -O2"}` |

### 6.6 benchmark_measurements

| Column | Type | Notes |
|---|---|---|
| `measurement_id` | BIGINT (identity) | Primary key |
| `benchmark_run_id` | BIGINT (FK → benchmark_runs) | Parent run |
| `iteration_number` | INTEGER | 1-indexed iteration within the run |
| `elapsed_us` | BIGINT | Latency for this iteration in microseconds |
| `result_count` | BIGINT | Results returned (used to verify correctness equivalence) |
| `memory_bytes` | BIGINT | Peak memory estimate for this iteration; 0 if not measured |

---

## 7. Normalization Analysis

### 7.1 Normal forms

The schema satisfies **Third Normal Form (3NF)**:

- **1NF:** Every column holds atomic values; no repeating groups. ✓
- **2NF:** Every non-key column depends on the whole primary key. In `document_terms`, `term_frequency` depends on `(document_id, term_id)` — the full composite key. ✓
- **3NF:** No transitive dependencies. `terms.document_frequency` depends on `terms.term_id`, not on any non-key column. `documents` columns are all directly dependent on `document_id`. ✓

### 7.2 Design decisions

- `updated_at` is a **stored column**, not a derived view, because it must be set at write time by the application. If a view computed it, writes to the view would require rules or triggers.
- **Term statistics** (`document_frequency`, `term_frequency`) are stored redundantly with respect to what could be derived by counting `document_terms` rows. This is a deliberate performance trade-off: on Trie reconstruction the application reads these values directly rather than running aggregation queries.
- **Derived attributes** (total match count across a search, average score for a query) are computed in the C++ application layer, not stored, to avoid staleness.

---

## 8. Key SQL Queries

### Insert a document (application uses parameterized form)

```sql
INSERT INTO documents (title, body, category, source)
VALUES ($1, $2, $3, $4)
RETURNING document_id;
```

### Get all documents for index reconstruction (startup)

```sql
SELECT document_id, title, body, category, source, created_at, updated_at
FROM documents
ORDER BY document_id ASC;
```

### Recent search history

```sql
SELECT query_text, query_type, result_count, elapsed_us, created_at
FROM search_history
ORDER BY created_at DESC
LIMIT $1;
```

### Most frequently searched queries

```sql
SELECT query_text, COUNT(*) AS search_count
FROM search_history
WHERE query_type = 'substring'
GROUP BY query_text
ORDER BY search_count DESC
LIMIT 10;
```

### Benchmark summary: average, best, worst per algorithm and operation

```sql
SELECT
    br.algorithm_name,
    br.operation_name,
    br.dataset_size,
    COUNT(bm.measurement_id)          AS iterations,
    AVG(bm.elapsed_us)                AS avg_us,
    MIN(bm.elapsed_us)                AS best_us,
    MAX(bm.elapsed_us)                AS worst_us,
    PERCENTILE_CONT(0.95)
        WITHIN GROUP (ORDER BY bm.elapsed_us)  AS p95_us
FROM benchmark_runs br
JOIN benchmark_measurements bm
    ON bm.benchmark_run_id = br.benchmark_run_id
GROUP BY br.algorithm_name, br.operation_name, br.dataset_size
ORDER BY br.operation_name, br.dataset_size, avg_us;
```

### Documents by category

```sql
SELECT category, COUNT(*) AS doc_count
FROM documents
WHERE category IS NOT NULL
GROUP BY category
ORDER BY doc_count DESC;
```

### Delete a document and cascade (application uses parameterized form)

```sql
DELETE FROM documents WHERE document_id = $1;
-- document_terms rows are removed automatically by ON DELETE CASCADE
```

---

## 9. Consistency Rules

- The application increments `terms.document_frequency` by 1 when a new document containing the term is inserted, and decrements it on deletion.
- The `term_frequency` in `document_terms` equals the number of times the normalized term appears in the document's body and title combined (or body only — define this and stay consistent).
- These statistics must be updated in the same transaction as the `documents` insert/update/delete.
- If an in-memory index update fails after a successful DB commit, the application marks indexes as `STALE` and refuses indexed search until a rebuild succeeds.

---

## 10. Setup Instructions

```bash
# 1. Create the database and user
createdb queryforge
psql -c "CREATE USER qf_user WITH PASSWORD 'changeme';"
psql -c "GRANT ALL PRIVILEGES ON DATABASE queryforge TO qf_user;"

# 2. Apply the schema
psql -U qf_user -d queryforge -f sql/schema.sql

# 3. Load seed data
psql -U qf_user -d queryforge -f sql/seed.sql

# 4. Set connection string for the application
export QUERYFORGE_DB_URL="host=localhost port=5432 dbname=queryforge user=qf_user password=changeme"
```

The application reads `QUERYFORGE_DB_URL` from the environment. Do not commit credentials to the repository. Add a `.env.example` with a placeholder value and list `.env` in `.gitignore`.
