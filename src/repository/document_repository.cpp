#include "repository/document_repository.hpp"
#include <stdexcept>

DocumentRepository::DocumentRepository(const std::string& connectionString)
    : conn_(std::make_unique<pqxx::connection>(connectionString)) {}

std::vector<Document> DocumentRepository::getAllDocuments() {
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT document_id, title, body, category, source "
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
        documents.push_back(std::move(doc));
    }

    txn.commit();
    return documents;
}

Document DocumentRepository::getDocumentById(long long id) {
    pqxx::work txn(*conn_);
    auto rows = txn.exec(
        "SELECT document_id, title, body, category, source "
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

    txn.commit();
    return doc;
}

long long DocumentRepository::insertDocument(const Document& doc) {
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
    if (normalizedTerm.empty() || termFrequency <= 0) {
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
    pqxx::work txn(*conn_);
    txn.exec(sql);
    txn.commit();
}
