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
    txn.exec(
        "DELETE FROM documents WHERE document_id = $1;",
        pqxx::params{id}
    );
    txn.commit();
}

void DocumentRepository::upsertTermStats(const std::string&, long long, int) {}

void DocumentRepository::deleteTermStats(long long) {}

void DocumentRepository::insertSearchHistory(const SearchHistoryEntry&) {}

std::vector<SearchHistoryEntry> DocumentRepository::getRecentSearches(int) {
    return {};
}

long long DocumentRepository::insertBenchmarkRun(const BenchmarkRun&) {
    return 0;
}

void DocumentRepository::insertBenchmarkMeasurement(const BenchmarkMeasurement&) {}

void DocumentRepository::executeNonQuery(const std::string& sql,
                                        const std::vector<std::string>&) {
    pqxx::work txn(*conn_);
    txn.exec(sql);
    txn.commit();
}
