#include "repository/document_repository.hpp"
#include <chrono>
#include <stdexcept>
#include <algorithm>
#include <iostream>

namespace {
std::vector<Document> initSeedDocuments() {
    auto now = std::chrono::system_clock::now();
    return {
        {1, "Introduction to Algorithms", "An algorithm is a step-by-step procedure for solving a problem. Common algorithms include sorting and searching.", "algorithms", "sample_documents", now, now},
        {2, "Binary Search Trees", "A binary search tree stores keys in sorted order. Each node has at most two children. Search, insert and delete all run in O(log n) average time.", "data-structures", "sample_documents", now, now},
        {3, "AVL Trees and Balancing", "An AVL tree is a self-balancing binary search tree. The balance factor of every node is maintained between -1 and 1 using rotations.", "data-structures", "sample_documents", now, now},
        {4, "Suffix Arrays for Pattern Matching", "A suffix array contains sorted suffixes of a string. Binary search over the array enables pattern matching in O(P log N) time.", "algorithms", "sample_documents", now, now},
        {5, "Trie Data Structure", "A trie is a tree-like structure for storing strings. It supports fast prefix queries and is commonly used for autocomplete systems.", "data-structures", "sample_documents", now, now},
        {6, "Heaps and Priority Queues", "A max-heap is a complete binary tree where every parent key is greater than its children. It supports extract-max in O(log n) time.", "data-structures", "sample_documents", now, now},
        {7, "Database Indexing", "Database indexes accelerate queries by maintaining sorted structures over column values. B-trees and hash indexes are common choices.", "databases", "sample_documents", now, now},
        {8, "Text Search Engines", "A full-text search engine builds inverted indexes mapping terms to documents. Scoring models such as TF-IDF rank results by relevance.", "search", "sample_documents", now, now},
        {9, "Graph Algorithms", "Graph algorithms traverse edges and vertices to solve problems. Breadth-first search and Dijkstra are widely used for path finding.", "algorithms", "sample_documents", now, now},
        {10, "Dynamic Programming", "Dynamic programming solves problems by breaking them into overlapping subproblems and caching intermediate results. Memoization is a common technique.", "algorithms", "sample_documents", now, now}
    };
}
static std::vector<Document> g_fallbackDocs = initSeedDocuments();
static std::vector<SearchHistoryEntry> g_fallbackHistory;
} // namespace

DocumentRepository::DocumentRepository(const std::string& connectionString) {
    if (!connectionString.empty()) {
        try {
            conn_ = std::make_unique<pqxx::connection>(connectionString);
            if (!conn_->is_open()) {
                conn_.reset();
            }
        } catch (const std::exception& e) {
            std::cerr << "[INFO] Database connection not active (" << e.what() << "). Using in-memory seed documents.\n";
            conn_.reset();
        }
    } else {
        conn_.reset();
    }
}

std::vector<Document> DocumentRepository::getAllDocuments() {
    if (!conn_) {
        return g_fallbackDocs;
    }
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT document_id, title, body, category, source, "
        "FLOOR(EXTRACT(EPOCH FROM created_at) * 1000000)::BIGINT AS created_at_us, "
        "FLOOR(EXTRACT(EPOCH FROM updated_at) * 1000000)::BIGINT AS updated_at_us "
        "FROM documents "
        "ORDER BY document_id ASC;"
    );

    std::vector<Document> documents;
    documents.reserve(rows.size());

    for (const auto& row : rows) {
        Document doc;
        doc.id = row["document_id"].as<long long>();
        doc.title = row["title"].c_str();
        doc.body = row["body"].c_str();
        doc.category = row["category"].is_null() ? "" : row["category"].c_str();
        doc.source = row["source"].is_null() ? "" : row["source"].c_str();
        doc.createdAt = std::chrono::system_clock::time_point{
            std::chrono::microseconds{row["created_at_us"].as<long long>()}};
        doc.updatedAt = std::chrono::system_clock::time_point{
            std::chrono::microseconds{row["updated_at_us"].as<long long>()}};
        documents.push_back(std::move(doc));
    }

    txn.commit();
    return documents;
}

Document DocumentRepository::getDocumentById(long long id) {
    if (!conn_) {
        for (const auto& d : g_fallbackDocs) {
            if (d.id == id) return d;
        }
        throw std::runtime_error("Document not found with id: " + std::to_string(id));
    }
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT document_id, title, body, category, source, "
        "FLOOR(EXTRACT(EPOCH FROM created_at) * 1000000)::BIGINT AS created_at_us, "
        "FLOOR(EXTRACT(EPOCH FROM updated_at) * 1000000)::BIGINT AS updated_at_us "
        "FROM documents "
        "WHERE document_id = $1;",
        pqxx::params{id}
    );

    if (rows.empty()) {
        throw std::runtime_error("Document not found with id: " + std::to_string(id));
    }

    const auto& row = rows[0];
    Document doc;
    doc.id = row["document_id"].as<long long>();
    doc.title = row["title"].c_str();
    doc.body = row["body"].c_str();
    doc.category = row["category"].is_null() ? "" : row["category"].c_str();
    doc.source = row["source"].is_null() ? "" : row["source"].c_str();
    doc.createdAt = std::chrono::system_clock::time_point{
        std::chrono::microseconds{row["created_at_us"].as<long long>()}};
    doc.updatedAt = std::chrono::system_clock::time_point{
        std::chrono::microseconds{row["updated_at_us"].as<long long>()}};

    txn.commit();
    return doc;
}

long long DocumentRepository::insertDocument(const Document& doc) {
    if (!conn_) {
        long long newId = g_fallbackDocs.empty() ? 1 : (g_fallbackDocs.back().id + 1);
        Document d = doc;
        d.id = newId;
        d.createdAt = std::chrono::system_clock::now();
        d.updatedAt = d.createdAt;
        g_fallbackDocs.push_back(d);
        return newId;
    }
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "INSERT INTO documents (title, body, category, source) "
        "VALUES ($1, $2, $3, $4) "
        "RETURNING document_id;",
        pqxx::params{doc.title, doc.body, doc.category, doc.source}
    );

    long long newId = rows[0]["document_id"].as<long long>();
    txn.commit();
    return newId;
}

void DocumentRepository::updateDocument(const Document& doc) {
    if (!conn_) {
        for (auto& d : g_fallbackDocs) {
            if (d.id == doc.id) {
                d.title = doc.title;
                d.body = doc.body;
                d.category = doc.category;
                d.source = doc.source;
                d.updatedAt = std::chrono::system_clock::now();
                return;
            }
        }
        return;
    }
    pqxx::work txn(*conn_);
    txn.exec(
        "UPDATE documents "
        "SET title = $1, body = $2, category = $3, source = $4, updated_at = CURRENT_TIMESTAMP "
        "WHERE document_id = $5;",
        pqxx::params{doc.title, doc.body, doc.category, doc.source, doc.id}
    );
    txn.commit();
}

void DocumentRepository::deleteDocument(long long id) {
    if (!conn_) {
        g_fallbackDocs.erase(
            std::remove_if(g_fallbackDocs.begin(), g_fallbackDocs.end(), [id](const Document& d) {
                return d.id == id;
            }),
            g_fallbackDocs.end()
        );
        return;
    }
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT term_id FROM document_terms WHERE document_id = $1;",
        pqxx::params{id}
    );

    for (const auto& row : rows) {
        long long termId = row["term_id"].as<long long>();
        txn.exec(
            "UPDATE terms SET document_frequency = GREATEST(0, document_frequency - 1) "
            "WHERE term_id = $1;",
            pqxx::params{termId}
        );
    }

    txn.exec(
        "DELETE FROM documents WHERE document_id = $1;",
        pqxx::params{id}
    );
    txn.commit();
}

void DocumentRepository::upsertTermStats(const std::string& normalizedTerm,
                                         long long documentId,
                                         int termFrequency) {
    if (!conn_ || normalizedTerm.empty() || termFrequency <= 0) {
        return;
    }

    pqxx::work txn(*conn_);
    txn.exec(
        "INSERT INTO terms (normalized_term, document_frequency) "
        "VALUES ($1, 0) "
        "ON CONFLICT (normalized_term) DO NOTHING;",
        pqxx::params{normalizedTerm}
    );

    auto termRows = txn.exec(
        "SELECT term_id FROM terms WHERE normalized_term = $1;",
        pqxx::params{normalizedTerm}
    );

    if (termRows.empty()) {
        txn.abort();
        return;
    }
    long long termId = termRows[0]["term_id"].as<long long>();

    auto checkRows = txn.exec(
        "SELECT 1 FROM document_terms WHERE document_id = $1 AND term_id = $2;",
        pqxx::params{documentId, termId}
    );

    if (checkRows.empty()) {
        txn.exec(
            "INSERT INTO document_terms (document_id, term_id, term_frequency) "
            "VALUES ($1, $2, $3);",
            pqxx::params{documentId, termId, termFrequency}
        );
        txn.exec(
            "UPDATE terms SET document_frequency = document_frequency + 1 WHERE term_id = $1;",
            pqxx::params{termId}
        );
    } else {
        txn.exec(
            "UPDATE document_terms SET term_frequency = $1 "
            "WHERE document_id = $2 AND term_id = $3;",
            pqxx::params{termFrequency, documentId, termId}
        );
    }

    txn.commit();
}

void DocumentRepository::deleteTermStats(long long documentId) {
    if (!conn_) return;
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT term_id FROM document_terms WHERE document_id = $1;",
        pqxx::params{documentId}
    );

    for (const auto& row : rows) {
        long long termId = row["term_id"].as<long long>();
        txn.exec(
            "UPDATE terms SET document_frequency = GREATEST(0, document_frequency - 1) "
            "WHERE term_id = $1;",
            pqxx::params{termId}
        );
    }

    txn.exec(
        "DELETE FROM document_terms WHERE document_id = $1;",
        pqxx::params{documentId}
    );

    txn.commit();
}

void DocumentRepository::insertSearchHistory(const SearchHistoryEntry& entry) {
    if (!conn_) {
        g_fallbackHistory.push_back(entry);
        return;
    }
    pqxx::work txn(*conn_);
    txn.exec(
        "INSERT INTO search_history (query_text, query_type, requested_limit, result_count, elapsed_us) "
        "VALUES ($1, $2, $3, $4, $5);",
        pqxx::params{
            entry.queryText,
            entry.queryType,
            entry.requestedLimit,
            entry.resultCount,
            entry.elapsedMicros
        }
    );
    txn.commit();
}

std::vector<SearchHistoryEntry> DocumentRepository::getRecentSearches(int limit) {
    if (!conn_) {
        std::vector<SearchHistoryEntry> res = g_fallbackHistory;
        if (static_cast<int>(res.size()) > limit) res.resize(limit);
        return res;
    }
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT query_text, query_type, requested_limit, result_count, elapsed_us "
        "FROM search_history "
        "ORDER BY created_at DESC, search_id DESC "
        "LIMIT $1;",
        pqxx::params{limit}
    );

    std::vector<SearchHistoryEntry> entries;
    entries.reserve(rows.size());

    for (const auto& row : rows) {
        SearchHistoryEntry entry;
        entry.queryText = row["query_text"].c_str();
        entry.queryType = row["query_type"].c_str();
        entry.requestedLimit = row["requested_limit"].as<int>();
        entry.resultCount = row["result_count"].as<int>();
        entry.elapsedMicros = row["elapsed_us"].as<long long>();
        entries.push_back(std::move(entry));
    }

    txn.commit();
    return entries;
}

long long DocumentRepository::insertBenchmarkRun(const BenchmarkRun& run) {
    if (!conn_) return 1;
    pqxx::work txn(*conn_);
    std::string configJson = run.configuration.empty() ? "{}" : run.configuration;
    auto rows = txn.exec(
        "INSERT INTO benchmark_runs (dataset_name, dataset_size, algorithm_name, operation_name, configuration) "
        "VALUES ($1, $2, $3, $4, $5::jsonb) "
        "RETURNING benchmark_run_id;",
        pqxx::params{
            run.datasetName,
            run.datasetSize,
            run.algorithmName,
            run.operationName,
            configJson
        }
    );

    long long runId = rows[0]["benchmark_run_id"].as<long long>();
    txn.commit();
    return runId;
}

void DocumentRepository::insertBenchmarkMeasurement(const BenchmarkMeasurement& m) {
    if (!conn_) return;
    pqxx::work txn(*conn_);
    txn.exec(
        "INSERT INTO benchmark_measurements (benchmark_run_id, iteration_number, elapsed_us, result_count, memory_bytes) "
        "VALUES ($1, $2, $3, $4, $5);",
        pqxx::params{
            m.benchmarkRunId,
            m.iterationNumber,
            m.elapsedMicros,
            m.resultCount,
            m.memoryBytes
        }
    );
    txn.commit();
}

void DocumentRepository::executeNonQuery(const std::string& sql,
                                        const std::vector<std::string>&) {
    if (!conn_) return;
    pqxx::work txn(*conn_);
    txn.exec(sql);
    txn.commit();
}
