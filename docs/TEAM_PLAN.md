# QueryForge — Team Plan

**Project:** QueryForge  
**Team:** Tejas (Project Lead) · Kanak · Varad  
**Branches:** `dev_tejas` · `dev_kanak` · `dev_varad`

---

## 1. Roles

| Member | Role | Primary folders |
|---|---|---|
| **Tejas** | Project Lead · Trie & Normalizer Lead | `include/models/`, `include/engine/normalizer.*`, `include/structures/trie.*`, `src/main.cpp`, `CMakeLists.txt`, `docs/`, `README.md` |
| **Kanak** | Suffix Array & Search Engine Lead | `include/structures/suffix_array.*`, `include/engine/search_engine.*`, `tests/integration_tests.cpp`, `benchmarks/benchmark_main.cpp` |
| **Varad** | AVL Tree · Max-Heap · Database Lead | `include/structures/avl_tree.*`, `include/structures/max_heap.*`, `include/repository/document_repository.*`, `sql/` |

**Merges:** Tejas (project lead) is the only person who merges pull requests into `main`. Never squash-merge: each person's commits must remain individually visible.

---

## 2. Independence Contract

Each member's work compiles and passes unit tests without the other members' code being present. This is achieved as follows:

- **Tejas (Trie + Normalizer):** Depends only on `<string>` and `<vector>` from the standard library. No DB. Tests run against pure C++ input strings. Tejas also commits the shared `include/models/` headers and `CMakeLists.txt` in T1, so Kanak and Varad can build against them.
- **Kanak (Suffix Array):** Depends only on the shared models and `<string>`. No Trie, no DB. Builds and tests as a standalone translation unit.
- **Varad (AVL Tree + Max-Heap + Repository):** AVL Tree depends only on `long long` keys; Max-Heap depends on `SearchResult` (shared model). Repository depends on libpqxx and shared models. All three test suites run independently.
- **Search Engine (Kanak):** Assembled last. Depends on Trie, Suffix Array, AVL Tree, and Max-Heap. Integrated only after all three members have merged their structures.

---

## 3. Commit Table (21 commits total — 7 per member)

### Tejas — `dev_tejas` (T1 – T7)

| ID | Commit message | Files | Depends on | Completion criterion |
|---|---|---|---|---|
| **T1** | `chore: cmake setup, shared models, project skeleton` | `CMakeLists.txt`, `include/models/document.hpp`, `include/models/search_result.hpp`, `include/models/search_history_entry.hpp`, `README.md`, `.gitignore` | Nothing | Project builds with `cmake .. && make`; shared headers compile cleanly |
| **T2** | `feat: implement text normalizer` | `include/engine/normalizer.hpp`, `src/engine/normalizer.cpp`, `tests/normalizer_tests.cpp` | T1 | All normalizer unit tests pass; tokenize and normalizeToken behave as per LLD contract |
| **T3** | `feat: implement trie with insert and search` | `include/structures/trie.hpp`, `src/structures/trie.cpp` | T1, T2 | `insertTerm`, `contains`, and `autocomplete` pass core tests |
| **T4** | `feat: add trie deletion and document-level remove` | Updated `trie.cpp`, `trie.hpp` | T3 | `removeTerm` and `removeDocument` pass all mutation tests |
| **T5** | `test: complete trie unit tests` | `tests/trie_tests.cpp` | T4 | All 8 trie test cases pass; differential test against linear scan passes |
| **T6** | `feat: implement cli command parser and main loop` | `src/main.cpp` | T1 | `queryforge help`, `queryforge stats` run without crashing; unknown commands show usage |
| **T7** | `feat: wire search engine into cli and add end-to-end demo` | Updated `src/main.cpp` | T6, K7, V7 (after integration merge) | All CLI commands produce correct output; demo sequence from Section 20 of project spec runs successfully |

### Kanak — `dev_kanak` (K1 – K7)

| ID | Commit message | Files | Depends on | Completion criterion |
|---|---|---|---|---|
| **K1** | `feat: implement suffix array construction with prefix doubling` | `include/structures/suffix_array.hpp`, `src/structures/suffix_array.cpp` | T1 (shared models) | `SuffixArray("hello world")` builds without error; `size()` returns text length |
| **K2** | `feat: add binary search pattern lookup to suffix array` | Updated `suffix_array.cpp` | K1 | `search("hello")` returns `{0}` for text `"hello world"`; multiple occurrences returned correctly |
| **K3** | `test: complete suffix array unit and differential tests` | `tests/suffix_array_tests.cpp` | K2 | All 9 suffix array test cases pass; differential comparison against `std::string::find` passes for 50 random strings |
| **K4** | `feat: implement search engine skeleton and index construction` | `include/engine/search_engine.hpp`, `src/engine/search_engine.cpp` | T2, T3, T4, K1, K2, V1, V2 | `buildIndexes()` loads documents, builds Trie, builds per-doc suffix arrays, and builds AVL Tree without errors |
| **K5** | `feat: implement substring search, scoring, and top-k ranking` | Updated `search_engine.cpp` | K4, V3, V4 | `search("algorithm")` returns a non-empty list of `SearchResult` ordered by score descending |
| **K6** | `feat: add autocomplete and range query to search engine` | Updated `search_engine.cpp` | K5 | `autocomplete("alg")` returns terms starting with "alg"; `rangeQuery(1, 5)` returns IDs 1–5 in sorted order |
| **K7** | `feat: implement benchmark runner and integration tests` | `benchmarks/benchmark_main.cpp`, `tests/integration_tests.cpp` | K5, K6, V5, V6 | Benchmark runs complete for all four structures; results are persisted to DB; integration tests for insert→search→delete cycle pass |

### Varad — `dev_varad` (V1 – V7)

| ID | Commit message | Files | Depends on | Completion criterion |
|---|---|---|---|---|
| **V1** | `feat: implement avl tree insert and search` | `include/structures/avl_tree.hpp`, `src/structures/avl_tree.cpp` | T1 (shared models) | `insert(5)`, `contains(5)` return expected results; `isValid()` passes after 20 inserts |
| **V2** | `feat: add avl tree deletion and range query` | Updated `avl_tree.cpp`, `avl_tree.hpp` | V1 | `remove()` and `rangeQuery()` work correctly; `isValid()` passes after deletions; all four rotation cases exercised |
| **V3** | `test: complete avl tree unit tests` | `tests/avl_tree_tests.cpp` | V2 | All AVL tree test cases (sorted, reverse, random, rotations, range) pass |
| **V4** | `feat: implement max-heap with top-k extraction` | `include/structures/max_heap.hpp`, `src/structures/max_heap.cpp`, `tests/max_heap_tests.cpp` | T1 | All max-heap test cases pass; differential test against `std::sort` passes for 30 random inputs; tie-breaking by document ID is deterministic |
| **V5** | `feat: implement postgresql schema and repository layer` | `sql/schema.sql`, `sql/seed.sql`, `include/repository/document_repository.hpp`, `src/repository/document_repository.cpp` | T1 | Schema applies cleanly; seed loads 10 documents; `getAllDocuments()` returns 10 rows; `insertDocument()` and `deleteDocument()` work with cascades |
| **V6** | `feat: add search history and benchmark persistence to repository` | Updated `document_repository.cpp` | V5 | `insertSearchHistory()` writes a row; `insertBenchmarkRun()` and `insertBenchmarkMeasurement()` write rows; rows are queryable |
| **V7** | `test: repository integration test and upsert term stats` | Updated `document_repository.cpp`, `tests/db_repository_tests.cpp` | V6 | Full insert→update→delete cycle verified with real DB; term statistics are updated in the same transaction; cascade deletes confirmed |

---

## 4. Merge Checkpoints

### Checkpoint 0 — Repository bootstrap (Tejas, on `main`)

Before any branch work begins:

```bash
git init -b main
git add docs README.md .gitignore
git commit -m "docs: add project design documents and setup files"
git remote add origin <repo-url>
git push -u origin main
```

This commit does not count towards the 21. Kanak and Varad clone and create their branches only after this push.

---

### Checkpoint 1 — Core structures ready (end of Week 2)

**Merge order into `main`:**

1. `dev_tejas` → main (T1 – T5: models, normalizer, Trie)
2. `dev_varad` → main (V1 – V4: AVL Tree, Max-Heap)
3. `dev_kanak` → main (K1 – K3: Suffix Array)

**Gate:** All unit tests pass on each branch before merging. No merge may break the build.

---

### Checkpoint 2 — Database and Search Engine integration (end of Week 4)

**Merge order:**

1. `dev_varad` → main (V5 – V6: schema, repository)
2. `dev_kanak` → main (K4 – K6: Search Engine)

**Gate:** `buildIndexes()` runs successfully against the seeded DB. `search("algorithm")` returns at least one result.

---

### Checkpoint 3 — Final integration (end of Week 5)

**Merge order:**

1. `dev_varad` → main (V7: repository tests)
2. `dev_kanak` → main (K7: benchmark runner, integration tests)
3. `dev_tejas` → main (T6 – T7: CLI, end-to-end wire-up)

**Gate:** All unit tests, integration tests, and benchmark runs complete without errors. Demo sequence (Section 20 of project spec) runs reproducibly.

---

## 5. Git Rules

1. **Never commit directly to `main`.** All work goes on personal branches.
2. **Only Tejas merges PRs into `main`**, using **Rebase and merge** or **Create a merge commit** — never Squash, so each person's 7 commits remain individually visible.
3. Commit messages follow the format: `type: short description` (types: `feat`, `fix`, `test`, `chore`, `docs`, `refactor`).
4. Each commit must build cleanly: `cmake .. && make` should succeed.
5. Run unit tests before opening a PR: `ctest --output-on-failure`.
6. Never force-push to `main`. Force-push on personal branches is allowed before a PR is opened.
7. Do not run `git commit` inside Codex or AI sessions — review the diff yourself and commit with your own name.

### Branch setup (each member)

```bash
git clone <repo-url>
cd queryforge
git config user.name "Your Name"
git config user.email "your-github-email"
git switch -c dev_tejas     # dev_kanak or dev_varad for others
git push -u origin dev_tejas
```

---

## 6. Codex/AI Prompt Template

Use one prompt per commit. Change only the commit ID line and the scope line. Never let Codex commit — review the diff yourself first.

```
Read docs/HLD.md, docs/LLD.md, docs/TEAM_PLAN.md.
I am [Your Name], [Your Role], branch [your branch].

STEP 1 — READ FIRST (do not skip):
- docs/TEAM_PLAN.md  (my commit table is in section 3)
- docs/LLD.md        (my interfaces are in section [N])
- docs/HLD.md        (architecture overview)

STEP 2 — IMPLEMENT ONLY COMMIT [ID]:
"[exact commit message from TEAM_PLAN section 3]"

Touch only files listed in the "Files" column for that commit.
Do not touch any other member's folders.

Rules:
- C++17 or C++20.
- No STL containers replacing the custom structures (no std::map, no std::priority_queue for the custom implementations).
- Use RAII and std::unique_ptr for heap-allocated nodes.
- Strict weak ordering for all comparators.
- Unit tests must link against Catch2 or GoogleTest and pass with ctest.
- Do not run git commit. I will review and commit myself.

STEP 3 — VERIFY:
Run cmake .. && make && ctest --output-on-failure inside the build/ directory.
Fix any compile or test errors before stopping.

STEP 4 — STOP:
Reply with a short list of files created or modified and which tests passed.
Do not start the next commit.
```

---

## 7. Weekly Roadmap

| Week | Tejas | Kanak | Varad |
|---|---|---|---|
| 1 | T1: CMake, shared models, repo bootstrap, docs push | — clone after T1 merges, read all docs, plan K1 | — clone after T1 merges, read all docs, plan V1 |
| 2 | T2–T5: Normalizer + Trie + Trie tests | K1–K3: Suffix Array + tests | V1–V4: AVL Tree + Max-Heap + tests |
| **CP1 merge** | **Checkpoint 1: all unit tests pass; Tejas merges all three branches** | | |
| 3 | T6: CLI parser and main loop | K4: Search Engine skeleton + buildIndexes | V5–V6: Schema, seed, repository, search history, benchmark persistence |
| **CP2 merge** | **Checkpoint 2: Tejas merges V5-V6 then K4-K6** | K5–K6: search, autocomplete, range query in SearchEngine | |
| 4 | T6 refinement; start T7 prep | K7: Benchmark runner + integration tests | V7: Repository integration tests + term stats upsert |
| **CP3 merge** | **Checkpoint 3: Tejas merges V7, K7, then T6-T7** | | |
| 5 | T7: Wire CLI to SearchEngine; final demo | Fix any integration bugs; help with benchmarks | Fix any DB/repo bugs; verify cascade deletes |
| 6 | Benchmark report, complexity table, report writeup | Finalize benchmark measurements and report section | Finalize schema diagram, ER description for report |
| 7 (optional) | Optional web UI or index visualization | Optional benchmark charts | Optional extended DB queries or JSONB analysis |

---

## 8. Decisions for the Team to Confirm

1. **Roles as assigned:** Tejas is Project Lead and Trie/Normalizer Lead. Kanak owns the Suffix Array and Search Engine. Varad owns the AVL Tree, Max-Heap, and all database work. Confirm or swap.

2. **Term statistics scope:** Currently `document_terms` counts occurrences in body only. If title tokens should also be counted, define this before T2 (normalizer) is committed, since the Trie and the repository must agree.

3. **Suffix Array scope:** The LLD specifies per-document suffix arrays. If the team decides to build a combined suffix array with a position-to-document mapping, update the Suffix Array interface in the LLD before K1.

4. **K default value:** `search()` defaults to `limit = 10`. If the instructor specifies a different K, update the CLI default in T6 and the search engine default in K5 before those commits are merged.

5. **Benchmark dataset source:** The team must agree on the dataset before K7. Options: (a) generate synthetic documents in C++, (b) download Project Gutenberg plain-text files, (c) use the sample_documents from the seed. Document the choice in `benchmarks/datasets/README.md`.
