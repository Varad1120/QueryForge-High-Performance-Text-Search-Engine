#include <catch2/catch_test_macros.hpp>

#include "engine/search_engine.hpp"

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

namespace {

std::vector<Document> integrationDocuments;

Document makeIntegrationDocument(long long id,
                                 const std::string& title,
                                 const std::string& body) {
    Document document;
    document.id = id;
    document.title = title;
    document.body = body;
    document.createdAt = std::chrono::system_clock::now();
    document.updatedAt = document.createdAt;
    return document;
}

class TestDocumentStore {
public:
    long long insert(Document document) {
        long long nextId = 1;
        for (const Document& existing : integrationDocuments) {
            nextId = std::max(nextId, existing.id + 1);
        }
        document.id = nextId;
        integrationDocuments.push_back(std::move(document));
        return nextId;
    }

    void update(const Document& document) {
        for (Document& existing : integrationDocuments) {
            if (existing.id == document.id) {
                existing = document;
                return;
            }
        }
        FAIL("Cannot update a missing test document");
    }

    void erase(long long id) {
        integrationDocuments.erase(
            std::remove_if(integrationDocuments.begin(), integrationDocuments.end(),
                           [id](const Document& document) { return document.id == id; }),
            integrationDocuments.end());
    }
};

}  // namespace

// Test-only repository methods keep this lifecycle test isolated from PostgreSQL.
DocumentRepository::DocumentRepository(const std::string&) {}

std::vector<Document> DocumentRepository::getAllDocuments() {
    return integrationDocuments;
}

void DocumentRepository::insertSearchHistory(const SearchHistoryEntry&) {}

TEST_CASE("SearchEngine rebuilds every index across insert, update, and delete", "[integration]") {
    integrationDocuments.clear();
    TestDocumentStore store;
    const long long searchableId = store.insert(
        makeIntegrationDocument(0, "Amber guide", "amberstone appears here"));

    DocumentRepository repository("");
    SearchEngine engine(repository);
    engine.buildIndexes();

    REQUIRE(engine.indexState() == IndexState::READY);
    REQUIRE(engine.documentCount() == 1);
    auto amberResults = engine.search("amberstone");
    REQUIRE(amberResults.size() == 1);
    REQUIRE(amberResults[0].documentId == searchableId);
    REQUIRE(engine.search("violetsearch").empty());
    REQUIRE(engine.autocomplete("amb", 10).size() == 2);
    REQUIRE(engine.rangeQuery(searchableId, searchableId) ==
            std::vector<long long>{searchableId});

    const long long nonmatchingId = store.insert(
        makeIntegrationDocument(0, "Violet reference", "violetsearch only"));
    engine.buildIndexes();
    REQUIRE(engine.documentCount() == 2);
    const auto violetResults = engine.search("violetsearch");
    REQUIRE(violetResults.size() == 1);
    REQUIRE(violetResults[0].documentId == nonmatchingId);
    amberResults = engine.search("amberstone");
    REQUIRE(amberResults.size() == 1);
    REQUIRE(amberResults[0].documentId == searchableId);
    REQUIRE(engine.rangeQuery(searchableId, nonmatchingId) ==
            std::vector<long long>{searchableId, nonmatchingId});

    Document revised = makeIntegrationDocument(
        searchableId, "Cobaltstone guide", "cobaltstone replaces the old term");
    store.update(revised);
    engine.buildIndexes();
    REQUIRE(engine.search("amberstone").empty());
    const auto cobaltResults = engine.search("cobaltstone");
    REQUIRE(cobaltResults.size() == 1);
    REQUIRE(cobaltResults[0].documentId == searchableId);
    REQUIRE(engine.autocomplete("amb", 10).empty());
    REQUIRE_FALSE(engine.autocomplete("cob", 10).empty());
    REQUIRE(engine.rangeQuery(searchableId, nonmatchingId) ==
            std::vector<long long>{searchableId, nonmatchingId});

    store.erase(searchableId);
    engine.buildIndexes();
    REQUIRE(engine.documentCount() == 1);
    REQUIRE(engine.search("cobaltstone").empty());
    REQUIRE(engine.search("violetsearch").size() == 1);
    REQUIRE(engine.autocomplete("cob", 10).empty());
    REQUIRE(engine.rangeQuery(searchableId, nonmatchingId) ==
            std::vector<long long>{nonmatchingId});
}
