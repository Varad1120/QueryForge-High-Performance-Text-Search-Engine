#include "repository/document_repository.hpp"
#include "structures/avl_tree.hpp"
#include "structures/max_heap.hpp"
#include "structures/suffix_array.hpp"
#include "structures/trie.hpp"

#include <pqxx/pqxx>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

constexpr std::size_t kDatasetSize = 512;
constexpr int kIterations = 5;
constexpr int kQueryOperations = 50;
constexpr std::size_t kTopK = 25;
constexpr unsigned int kDatasetSeed = 20261028;
constexpr const char* kDatasetName = "synthetic_sequential_terms_v1";

struct Measurement {
    int iterationNumber;
    long long elapsedMicros;
    long long resultCount;
    long long memoryBytes{0};  // Not measured by this benchmark.
};

struct OperationResults {
    std::string algorithm;
    std::string operation;
    long long datasetSize;
    long long operationsPerMeasurement;
    long long textBytes;
    std::vector<Measurement> measurements;
};

std::vector<std::string> makeTerms() {
    std::vector<std::string> terms;
    terms.reserve(kDatasetSize);
    for (std::size_t i = 0; i < kDatasetSize; ++i) {
        std::ostringstream term;
        term << "token" << std::setw(4) << std::setfill('0') << i;
        terms.push_back(term.str());
    }
    return terms;
}

std::string joinTerms(const std::vector<std::string>& terms) {
    std::size_t length = terms.empty() ? 0 : terms.size() - 1;
    for (const std::string& term : terms) {
        length += term.size();
    }

    std::string text;
    text.reserve(length);
    for (const std::string& term : terms) {
        if (!text.empty()) {
            text.push_back(' ');
        }
        text.append(term);
    }
    return text;
}

template <typename Operation>
Measurement measure(int iteration, Operation&& operation) {
    const auto start = Clock::now();
    const long long resultCount = operation();
    const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now() - start);
    return {iteration, elapsed.count(), resultCount, 0};
}

OperationResults makeResult(std::string algorithm,
                            std::string operation,
                            long long operationsPerMeasurement,
                            long long textBytes = 0) {
    return {std::move(algorithm), std::move(operation),
            static_cast<long long>(kDatasetSize), operationsPerMeasurement,
            textBytes, {}};
}

std::string makeConfiguration(const std::string& operation,
                              long long operationsPerMeasurement,
                              long long textBytes) {
    std::ostringstream json;
    json << "{\"dataset\":\"" << kDatasetName
         << "\",\"seed\":" << kDatasetSeed
         << ",\"dataset_size\":" << kDatasetSize
         << ",\"iterations\":" << kIterations
         << ",\"operation\":\"" << operation
         << "\",\"operations_per_measurement\":" << operationsPerMeasurement;

    json << ",\"text_bytes\":" << textBytes
         << ",\"top_k\":" << kTopK
         << ",\"memory_bytes_recorded\":false}";
    return json.str();
}

void persistAndPrint(const OperationResults& results,
                     DocumentRepository& repository,
                     pqxx::connection& verificationConnection) {
    BenchmarkRun run;
    run.datasetName = kDatasetName;
    run.datasetSize = results.datasetSize;
    run.algorithmName = results.algorithm;
    run.operationName = results.operation;
    run.configuration = makeConfiguration(results.operation,
                                          results.operationsPerMeasurement,
                                          results.textBytes);

    const long long runId = repository.insertBenchmarkRun(run);
    for (const Measurement& measurement : results.measurements) {
        BenchmarkMeasurement record;
        record.benchmarkRunId = runId;
        record.iterationNumber = measurement.iterationNumber;
        record.elapsedMicros = measurement.elapsedMicros;
        record.resultCount = measurement.resultCount;
        record.memoryBytes = measurement.memoryBytes;
        repository.insertBenchmarkMeasurement(record);
    }

    pqxx::read_transaction verify(verificationConnection);
    const auto runRows = verify.exec(
        "SELECT dataset_name, algorithm_name, operation_name "
        "FROM benchmark_runs WHERE benchmark_run_id = $1;",
        pqxx::params{runId});
    const auto measurementRows = verify.exec(
        "SELECT COUNT(*) AS measurement_count "
        "FROM benchmark_measurements WHERE benchmark_run_id = $1;",
        pqxx::params{runId});
    if (runRows.size() != 1 ||
        runRows[0]["dataset_name"].c_str() != std::string(kDatasetName) ||
        runRows[0]["algorithm_name"].c_str() != results.algorithm ||
        runRows[0]["operation_name"].c_str() != results.operation ||
        measurementRows[0]["measurement_count"].as<long long>() != kIterations) {
        throw std::runtime_error("Persisted benchmark rows failed read-back verification");
    }
    verify.commit();

    const long long totalMicros = std::accumulate(
        results.measurements.begin(), results.measurements.end(), 0LL,
        [](long long total, const Measurement& measurement) {
            return total + measurement.elapsedMicros;
        });
    const double meanMicros = static_cast<double>(totalMicros) /
                              static_cast<double>(results.measurements.size());

    std::cout << std::left << std::setw(12) << results.algorithm
              << std::setw(20) << results.operation
              << "dataset=" << results.datasetSize
              << " iterations=" << results.measurements.size()
              << " ops/sample=" << results.operationsPerMeasurement
              << " mean_us=" << std::fixed << std::setprecision(2) << meanMicros
              << " persisted_run_id=" << runId
              << " verified_measurements=" << results.measurements.size()
              << '\n';
}

std::vector<OperationResults> runBenchmarks(const std::vector<std::string>& terms,
                                            const std::string& text) {
    std::vector<OperationResults> results;
    results.reserve(8);

    OperationResults trieBuild = makeResult("Trie", "insert_terms", kDatasetSize);
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        trieBuild.measurements.push_back(measure(iteration, [&]() {
            Trie trie;
            for (std::size_t i = 0; i < terms.size(); ++i) {
                trie.insertTerm(terms[i], static_cast<long long>(i + 1));
            }
            return static_cast<long long>(trie.uniqueTermCount());
        }));
    }
    results.push_back(std::move(trieBuild));

    Trie trie;
    for (std::size_t i = 0; i < terms.size(); ++i) {
        trie.insertTerm(terms[i], static_cast<long long>(i + 1));
    }
    OperationResults trieQuery = makeResult("Trie", "autocomplete_prefix", kQueryOperations);
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        trieQuery.measurements.push_back(measure(iteration, [&]() {
            long long count = 0;
            for (int operation = 0; operation < kQueryOperations; ++operation) {
                count += static_cast<long long>(trie.autocomplete("token00", 100).size());
            }
            return count;
        }));
    }
    results.push_back(std::move(trieQuery));

    OperationResults suffixBuild = makeResult("SuffixArray", "construct", 1,
                                               static_cast<long long>(text.size()));
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        suffixBuild.measurements.push_back(measure(iteration, [&]() {
            SuffixArray suffixArray(text);
            return static_cast<long long>(suffixArray.size());
        }));
    }
    results.push_back(std::move(suffixBuild));

    SuffixArray suffixArray(text);
    OperationResults suffixSearch = makeResult("SuffixArray", "search_pattern",
                                                 kQueryOperations,
                                                 static_cast<long long>(text.size()));
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        suffixSearch.measurements.push_back(measure(iteration, [&]() {
            long long count = 0;
            for (int operation = 0; operation < kQueryOperations; ++operation) {
                count += static_cast<long long>(suffixArray.search("token0256").size());
            }
            return count;
        }));
    }
    results.push_back(std::move(suffixSearch));

    std::vector<long long> keys(kDatasetSize);
    std::iota(keys.begin(), keys.end(), 1LL);
    std::mt19937 generator(kDatasetSeed);
    std::shuffle(keys.begin(), keys.end(), generator);

    OperationResults avlBuild = makeResult("AVLTree", "insert_keys", kDatasetSize);
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        avlBuild.measurements.push_back(measure(iteration, [&]() {
            AVLTree tree;
            for (const long long key : keys) {
                tree.insert(key);
            }
            return static_cast<long long>(tree.inOrder().size());
        }));
    }
    results.push_back(std::move(avlBuild));

    AVLTree avlTree;
    for (const long long key : keys) {
        avlTree.insert(key);
    }
    OperationResults avlQuery = makeResult("AVLTree", "range_query", kQueryOperations);
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        avlQuery.measurements.push_back(measure(iteration, [&]() {
            long long count = 0;
            for (int operation = 0; operation < kQueryOperations; ++operation) {
                count += static_cast<long long>(avlTree.rangeQuery(128, 384).size());
            }
            return count;
        }));
    }
    results.push_back(std::move(avlQuery));

    std::vector<SearchResult> candidates;
    candidates.reserve(kDatasetSize);
    for (std::size_t i = 0; i < kDatasetSize; ++i) {
        SearchResult result;
        result.documentId = static_cast<long long>(i + 1);
        result.title = terms[i];
        result.score = static_cast<double>(kDatasetSize - i);
        result.matchCount = 1;
        candidates.push_back(std::move(result));
    }

    OperationResults heapBuild = makeResult("MaxHeap", "build_heap", kDatasetSize);
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        heapBuild.measurements.push_back(measure(iteration, [&]() {
            MaxHeap heap(candidates);
            return static_cast<long long>(heap.size());
        }));
    }
    results.push_back(std::move(heapBuild));

    OperationResults heapExtract = makeResult("MaxHeap", "extract_top_k", kQueryOperations);
    for (int iteration = 1; iteration <= kIterations; ++iteration) {
        std::vector<MaxHeap> preparedHeaps;
        preparedHeaps.reserve(kQueryOperations);
        for (int operation = 0; operation < kQueryOperations; ++operation) {
            preparedHeaps.emplace_back(candidates);
        }
        heapExtract.measurements.push_back(measure(iteration, [&]() {
            long long count = 0;
            for (MaxHeap& heap : preparedHeaps) {
                count += static_cast<long long>(heap.extractTopK(kTopK).size());
            }
            return count;
        }));
    }
    results.push_back(std::move(heapExtract));

    return results;
}

}  // namespace

int main() {
    const char* databaseUrl = std::getenv("QUERYFORGE_DB_URL");
    if (databaseUrl == nullptr || databaseUrl[0] == '\0') {
        std::cerr << "QUERYFORGE_DB_URL is required. Benchmark execution is "
                     "blocked until a disposable, schema-initialized test database is configured; "
                     "no timings will be produced or silently left unpersisted.\n";
        std::cerr.flush();
        std::_Exit(2);
    }

    try {
        const std::string connectionString(databaseUrl);
        DocumentRepository repository(connectionString);
        pqxx::connection verificationConnection(connectionString);
        const auto terms = makeTerms();
        const std::string text = joinTerms(terms);
        const auto results = runBenchmarks(terms, text);

        std::cout << "Dataset: " << kDatasetName
                  << " (" << kDatasetSize << " sequential terms, seed " << kDatasetSeed
                  << "), text_bytes=" << text.size()
                  << ", iterations=" << kIterations
                  << ", memory usage not measured.\n";
        for (const OperationResults& operation : results) {
            persistAndPrint(operation, repository, verificationConnection);
        }
    } catch (const std::exception& error) {
        std::cerr << "Benchmark failed: " << error.what() << '\n';
        std::cerr.flush();
        std::_Exit(1);
    }

    std::cout.flush();
    std::cerr.flush();
    std::_Exit(0);
}
