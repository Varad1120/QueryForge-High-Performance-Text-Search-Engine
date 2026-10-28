# QueryForge — High-Performance Text Search Engine

QueryForge is an in-memory, high-performance text-search engine built using Advanced Data Structures in C++ (C++17/C++20) and backed by PostgreSQL for persistence.

## Team & Roles

| Member | Role | Core Responsibility |
|---|---|---|
| **Tejas** | Project Lead · Trie & Normalizer Lead | Models, Normalizer, Trie, CLI entry point, CMake build setup |
| **Kanak** | Suffix Array & Search Engine Lead | Suffix Array, Search Engine orchestration, Benchmarking |
| **Varad** | AVL Tree · Max-Heap · Database Lead | AVL Tree, Max-Heap, PostgreSQL repository layer |

---

## Core Data Structures

- **Trie (`include/structures/trie.hpp`)**: Prefix autocomplete and vocabulary management with document frequency tracking.
- **Suffix Array (`include/structures/suffix_array.hpp`)**: Substring pattern matching via prefix-doubling suffix array construction and binary search.
- **AVL Tree (`include/structures/avl_tree.hpp`)**: Self-balancing binary search tree for ordered metadata and range queries.
- **Max-Heap (`include/structures/max_heap.hpp`)**: Top-K result extraction with custom multi-factor scoring.

---

## Building the Project

### Prerequisites
- Modern C++ compiler supporting C++17 or C++20 (`clang++` or `g++`)
- CMake (>= 3.16)
- PostgreSQL & `libpqxx` (for repository persistence)
- `Catch2` (v3) for unit and integration testing

### Build Instructions

```bash
mkdir -p build && cd build
cmake ..
make -j
```

### Running Tests

```bash
ctest --output-on-failure
```

---

## CLI Usage

```bash
# Display help and available commands
./bin/queryforge help

# View system and index statistics
./bin/queryforge stats

# Substring search
./bin/queryforge search "<query>" --limit 10

# Prefix autocomplete
./bin/queryforge autocomplete "<prefix>"

# Range query on document IDs
./bin/queryforge range <fromId> <toId>
```
