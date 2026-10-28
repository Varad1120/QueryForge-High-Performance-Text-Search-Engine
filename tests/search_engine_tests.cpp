#include <catch2/catch_test_macros.hpp>

#include "engine/search_engine.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>
#include <vector>

namespace {

std::vector<Document> repositoryDocuments;
std::vector<SearchHistoryEntry> persistedHistory;

Document makeDocument(long long id,
                      const std::string& title,
                      const std::string& body,
                      std::chrono::system_clock::time_point createdAt =
                          std::chrono::system_clock::now()) {
    Document document;
    document.id = id;
    document.title = title;
    document.body = body;
    document.createdAt = createdAt;
    document.updatedAt = createdAt;
    return document;
}

}  // namespace

// These test-only repository definitions keep the suite independent of PostgreSQL.
DocumentRepository::DocumentRepository(const std::string&) {}

std::vector<Document> DocumentRepository::getAllDocuments() {
    return repositoryDocuments;
}

void DocumentRepository::insertSearchHistory(const SearchHistoryEntry& entry) {
    persistedHistory.push_back(entry);
}

TEST_CASE("SearchEngine normalizes queries and counts repeated body matches", "[search_engine]") {
    repositoryDocuments = {
        makeDocument(10, "Algorithm notes", "algorithm-algorithm, algorithm; banana banana")
    };
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    const auto results = engine.search("ALGORITHM!!!");
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].documentId == 10);
    REQUIRE(results[0].matchCount == 3);
    const double expectedScore = 5.0 + std::log1p(3.0) + 0.5;
    REQUIRE(std::abs(results[0].score - expectedScore) < 0.001);
    const auto overlapping = engine.search("ana");
    REQUIRE(overlapping.size() == 1);
    REQUIRE(overlapping[0].matchCount == 4);
    REQUIRE(persistedHistory.size() == 2);
    REQUIRE(persistedHistory[0].requestedLimit == 10);
}

TEST_CASE("SearchEngine ranks by score and breaks score ties by document ID", "[search_engine]") {
    const auto createdAt = std::chrono::system_clock::now() - std::chrono::hours(24 * 20);
    repositoryDocuments = {
        makeDocument(2, "Algorithm", "algorithm", createdAt),
        makeDocument(1, "Algorithm", "algorithm", createdAt),
        makeDocument(4, "Unrelated", "algorithm", createdAt)
    };
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    const auto results = engine.search("algorithm");
    REQUIRE(results.size() == 3);
    REQUIRE(results[0].documentId == 1);
    REQUIRE(results[1].documentId == 2);
    REQUIRE(results[0].score == results[1].score);
    const double expectedRecentScore =
        5.0 + std::log1p(1.0) + 0.5 * (1.0 - 20.0 / 365.0);
    REQUIRE(std::abs(results[0].score - expectedRecentScore) < 0.001);
    REQUIRE(results[2].documentId == 4);
    REQUIRE(results[0].score >= results[1].score);
    REQUIRE(results[1].score >= results[2].score);

    const auto limited = engine.search("algorithm", 2);
    REQUIRE(limited.size() == 2);
    REQUIRE(limited[0].documentId == 1);
    REQUIRE(limited[1].documentId == 2);
}

TEST_CASE("SearchEngine creates snippets around matches near either body boundary", "[search_engine]") {
    repositoryDocuments = {
        makeDocument(1, "Start", "needle " + std::string(240, 'x')),
        makeDocument(2, "End", std::string(240, 'y') + " needle"),
        makeDocument(3, "Short", "around needle")
    };
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    const auto results = engine.search("needle", 10);
    REQUIRE(results.size() == 3);
    for (const auto& result : results) {
        REQUIRE(result.snippet.size() <= 180);
        REQUIRE(result.snippet.find("needle") != std::string::npos);
    }
    const auto startSnippet = std::find_if(results.begin(), results.end(), [](const auto& result) {
        return result.documentId == 1;
    });
    const auto endSnippet = std::find_if(results.begin(), results.end(), [](const auto& result) {
        return result.documentId == 2;
    });
    REQUIRE(startSnippet != results.end());
    REQUIRE(endSnippet != results.end());
    REQUIRE(startSnippet->snippet.size() == 180);
    REQUIRE(endSnippet->snippet.size() == 180);
    const auto shortDocument = std::find_if(results.begin(), results.end(), [](const auto& result) {
        return result.documentId == 3;
    });
    REQUIRE(shortDocument != results.end());
    REQUIRE(shortDocument->snippet == "around needle");
}

TEST_CASE("SearchEngine handles no matches and empty normalized queries", "[search_engine]") {
    repositoryDocuments = {makeDocument(1, "Algorithm", "algorithm")};
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    REQUIRE(engine.search("missing").empty());
    REQUIRE(engine.search("!!!").empty());
    REQUIRE(persistedHistory.size() == 2);
    REQUIRE(persistedHistory[0].resultCount == 0);
    REQUIRE(persistedHistory[1].queryText == "!!!");
}

TEST_CASE("SearchEngine validates limits and requires ready indexes", "[search_engine]") {
    repositoryDocuments = {makeDocument(1, "Algorithm", "algorithm")};
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine unbuilt(repository);
    REQUIRE_THROWS_AS(unbuilt.search("algorithm"), std::logic_error);

    unbuilt.buildIndexes();
    REQUIRE_THROWS_AS(unbuilt.search("algorithm", 0), std::invalid_argument);
    REQUIRE_THROWS_AS(unbuilt.search("algorithm", -1), std::invalid_argument);
    REQUIRE(persistedHistory.empty());
}

TEST_CASE("SearchEngine persists search metadata through the repository", "[search_engine]") {
    repositoryDocuments = {makeDocument(1, "Algorithm", "algorithm algorithm")};
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    const auto results = engine.search("algorithm", 4);
    REQUIRE(results.size() == 1);
    REQUIRE(persistedHistory.size() == 1);
    REQUIRE(persistedHistory[0].queryText == "algorithm");
    REQUIRE(persistedHistory[0].queryType == "substring");
    REQUIRE(persistedHistory[0].requestedLimit == 4);
    REQUIRE(persistedHistory[0].resultCount == 1);
    REQUIRE(persistedHistory[0].elapsedMicros >= 0);
}

TEST_CASE("SearchEngine autocomplete normalizes and returns unique matching prefixes", "[search_engine][k6]") {
    repositoryDocuments = {
        makeDocument(1, "Algorithm", "algorithm algorithms algebra cat cater"),
        makeDocument(2, "Algorithms", "algorithm algorithmic algebraic catalog")
    };
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    const auto suggestions = engine.autocomplete("ALG", 10);
    REQUIRE(suggestions.size() == 5);
    for (const auto& suggestion : suggestions) {
        REQUIRE(suggestion.term.starts_with("alg"));
    }
    REQUIRE(suggestions[0].term == "algorithm");
    REQUIRE(suggestions[0].documentFreq == 2);
    REQUIRE(suggestions[1].term == "algorithms");
    REQUIRE(suggestions[1].documentFreq == 2);
    REQUIRE(suggestions[2].term == "algebra");
    REQUIRE(suggestions[3].term == "algebraic");
    REQUIRE(suggestions[4].term == "algorithmic");
    REQUIRE(engine.autocomplete("missing").empty());
    const auto exactTermPrefix = engine.autocomplete("cat", 10);
    REQUIRE(exactTermPrefix.size() == 3);
    REQUIRE(exactTermPrefix[0].term == "cat");
    REQUIRE(exactTermPrefix[1].term == "catalog");
    REQUIRE(exactTermPrefix[2].term == "cater");
    REQUIRE(engine.autocomplete("", 2).size() == 2);
    REQUIRE(engine.autocomplete("", 0).empty());
}

TEST_CASE("SearchEngine range query returns existing document IDs inclusively in order", "[search_engine][k6]") {
    repositoryDocuments = {
        makeDocument(5, "Five", "five"),
        makeDocument(1, "One", "one"),
        makeDocument(7, "Seven", "seven"),
        makeDocument(3, "Three", "three")
    };
    persistedHistory.clear();

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    REQUIRE(engine.rangeQuery(1, 5) == std::vector<long long>{1, 3, 5});
    REQUIRE(engine.rangeQuery(2, 4) == std::vector<long long>{3});
    REQUIRE(engine.rangeQuery(3, 3) == std::vector<long long>{3});
    REQUIRE(engine.rangeQuery(4, 4).empty());
    REQUIRE(engine.rangeQuery(5, 1).empty());
}

TEST_CASE("SearchEngine autocomplete and range query require ready indexes", "[search_engine][k6]") {
    DocumentRepository repository("");
    SearchEngine engine(repository);

    REQUIRE_THROWS_AS(engine.autocomplete("alg"), std::logic_error);
    REQUIRE_THROWS_AS(engine.rangeQuery(1, 5), std::logic_error);
}
