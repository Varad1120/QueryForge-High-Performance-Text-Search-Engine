#include <catch2/catch_test_macros.hpp>
#include "repository/document_repository.hpp"
#include <cstdlib>
#include <string>

static std::string getTestDbUrl() {
    const char* envUrl = std::getenv("QUERYFORGE_DB_URL");
    if (envUrl && std::string(envUrl).length() > 0) {
        return std::string(envUrl);
    }
    return "host=localhost port=5432 dbname=queryforge user=root password=Password@123";
}

TEST_CASE("DocumentRepository connects and fetches seeded documents", "[db_repository]") {
    DocumentRepository repo(getTestDbUrl());
    auto docs = repo.getAllDocuments();
    REQUIRE(docs.size() >= 10);

    auto firstDoc = repo.getDocumentById(docs[0].id);
    REQUIRE(firstDoc.id == docs[0].id);
    REQUIRE_FALSE(firstDoc.title.empty());
    REQUIRE_FALSE(firstDoc.body.empty());

    REQUIRE_THROWS_AS(repo.getDocumentById(99999999), std::runtime_error);
}

TEST_CASE("DocumentRepository executes document CRUD cycle", "[db_repository]") {
    DocumentRepository repo(getTestDbUrl());

    Document doc;
    doc.title = "CRUD Test Document";
    doc.body = "Body of the CRUD test document.";
    doc.category = "testing";
    doc.source = "integration_test";

    long long docId = repo.insertDocument(doc);
    REQUIRE(docId > 0);

    auto fetched = repo.getDocumentById(docId);
    REQUIRE(fetched.title == "CRUD Test Document");
    REQUIRE(fetched.body == "Body of the CRUD test document.");
    REQUIRE(fetched.category == "testing");
    REQUIRE(fetched.source == "integration_test");

    fetched.title = "Updated CRUD Document";
    fetched.body = "Updated body of document.";
    repo.updateDocument(fetched);

    auto updated = repo.getDocumentById(docId);
    REQUIRE(updated.title == "Updated CRUD Document");
    REQUIRE(updated.body == "Updated body of document.");

    repo.deleteDocument(docId);
    REQUIRE_THROWS_AS(repo.getDocumentById(docId), std::runtime_error);
}

TEST_CASE("DocumentRepository manages term statistics and cascades", "[db_repository]") {
    DocumentRepository repo(getTestDbUrl());

    Document doc;
    doc.title = "Term Stats Document";
    doc.body = "Testing term stats and cascade deletes.";
    doc.category = "stats";
    doc.source = "test";

    long long docId = repo.insertDocument(doc);
    REQUIRE(docId > 0);

    repo.upsertTermStats("testingterm", docId, 5);
    repo.upsertTermStats("testingterm", docId, 8);

    repo.deleteDocument(docId);
    REQUIRE_THROWS_AS(repo.getDocumentById(docId), std::runtime_error);
}

TEST_CASE("DocumentRepository logs and retrieves search history", "[db_repository]") {
    DocumentRepository repo(getTestDbUrl());

    SearchHistoryEntry entry;
    entry.queryText = "integration_search_test";
    entry.queryType = "substring";
    entry.requestedLimit = 10;
    entry.resultCount = 2;
    entry.elapsedMicros = 850;

    repo.insertSearchHistory(entry);

    auto recent = repo.getRecentSearches(10);
    REQUIRE_FALSE(recent.empty());
    REQUIRE(recent[0].queryText == "integration_search_test");
    REQUIRE(recent[0].queryType == "substring");
    REQUIRE(recent[0].requestedLimit == 10);
    REQUIRE(recent[0].resultCount == 2);
    REQUIRE(recent[0].elapsedMicros == 850);
}

TEST_CASE("DocumentRepository persists benchmark runs and measurements", "[db_repository]") {
    DocumentRepository repo(getTestDbUrl());

    BenchmarkRun run;
    run.datasetName = "integration_dataset";
    run.datasetSize = 100;
    run.algorithmName = "AVLTree";
    run.operationName = "rangeQuery";
    run.configuration = "{\"range\": [10, 50]}";

    long long runId = repo.insertBenchmarkRun(run);
    REQUIRE(runId > 0);

    BenchmarkMeasurement m;
    m.benchmarkRunId = runId;
    m.iterationNumber = 1;
    m.elapsedMicros = 320;
    m.resultCount = 41;
    m.memoryBytes = 1024 * 128;

    REQUIRE_NOTHROW(repo.insertBenchmarkMeasurement(m));
}
