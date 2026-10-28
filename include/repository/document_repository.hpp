#pragma once
#include "models/document.hpp"
#include "models/search_result.hpp"
#include "models/search_history_entry.hpp"
#include <vector>
#include <string>
#include <memory>
#include <pqxx/pqxx>

struct BenchmarkRun {
    std::string datasetName;
    long long   datasetSize{0};
    std::string algorithmName;
    std::string operationName;
    std::string configuration;
};

struct BenchmarkMeasurement {
    long long benchmarkRunId{0};
    int       iterationNumber{0};
    long long elapsedMicros{0};
    long long resultCount{0};
    long long memoryBytes{0};
};

class DocumentRepository {
public:
    explicit DocumentRepository(const std::string& connectionString);
    ~DocumentRepository() = default;

    std::vector<Document> getAllDocuments();
    Document              getDocumentById(long long id);
    long long             insertDocument(const Document& doc);
    void                  updateDocument(const Document& doc);
    void                  deleteDocument(long long id);

    void upsertTermStats(const std::string& normalizedTerm,
                         long long documentId,
                         int termFrequency);
    void deleteTermStats(long long documentId);

    void insertSearchHistory(const SearchHistoryEntry& entry);
    std::vector<SearchHistoryEntry> getRecentSearches(int limit = 50);

    long long insertBenchmarkRun(const BenchmarkRun& run);
    void      insertBenchmarkMeasurement(const BenchmarkMeasurement& m);

private:
    std::unique_ptr<pqxx::connection> conn_;

    void executeNonQuery(const std::string& sql,
                         const std::vector<std::string>& params = {});
};
