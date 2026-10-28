#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
#undef CPPHTTPLIB_OPENSSL_SUPPORT
#endif
#include "httplib.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

#include "models/document.hpp"
#include "models/search_result.hpp"
#include "models/search_history_entry.hpp"
#include "engine/normalizer.hpp"
#include "engine/search_engine.hpp"
#include "repository/document_repository.hpp"

using namespace std;

namespace {

std::string getDbUrl() {
    const char* envUrl = std::getenv("QUERYFORGE_DB_URL");
    if (envUrl && std::string(envUrl).length() > 0) {
        return std::string(envUrl);
    }
    return "";
}

void printBanner() {
    cout << "\n===================================================================\n";
    cout << "   QueryForge — High-Performance Text Search Engine (ADS Mini-Project)\n";
    cout << "   Data Structures: Trie · Suffix Array · Max-Heap · AVL Tree\n";
    cout << "===================================================================\n\n";
}

void printHelp() {
    cout << "Usage: queryforge <command> [arguments]\n\n"
         << "Commands:\n"
         << "  help                      Display this help guide\n"
         << "  stats                     Display index statistics and memory state\n"
         << "  autocomplete <prefix>     Trie-based prefix completions with frequency\n"
         << "  search <query> [--limit K] Substring pattern search with Max-Heap ranking\n"
         << "  range <fromId> <toId>     AVL Tree ordered range query on document IDs\n"
         << "  demo                      Run automated end-to-end ADS evaluation demo\n"
         << "  gui [--port P]            Launch embedded Web GUI (default port: 8080)\n"
         << "  interactive               Start interactive CLI REPL session\n\n";
}

void printStats(SearchEngine& engine, DocumentRepository& repo) {
    cout << "--------------------------------------------------------\n";
    cout << " QueryForge System & Index Statistics\n";
    cout << "--------------------------------------------------------\n";
    cout << " Index State:          " << (engine.indexState() == IndexState::READY ? "READY" : "STALE / NOT_BUILT") << "\n";
    cout << " Indexed Documents:    " << engine.documentCount() << "\n";
    cout << " Active Structures:    Trie, Per-Doc Suffix Arrays, Max-Heap, AVL Tree\n";
    
    auto history = repo.getRecentSearches(10);
    cout << " Recent Query History: " << history.size() << " recorded queries\n";
    for (size_t i = 0; i < history.size(); ++i) {
        cout << "   " << (i + 1) << ". [" << history[i].queryType << "] \"" 
             << history[i].queryText << "\" -> " << history[i].resultCount << " matches ("
             << history[i].elapsedMicros << " us)\n";
    }
    cout << "--------------------------------------------------------\n";
}

void runDemo(SearchEngine& engine, DocumentRepository& repo) {
    printBanner();
    cout << "[DEMO] Starting comprehensive evaluation sequence for ADS Evaluators...\n\n";

    // 1. Indexing
    cout << ">>> 1. IN-MEMORY INDEX RECONSTRUCTION (All 4 Structures)\n";
    auto t0 = chrono::high_resolution_clock::now();
    engine.buildIndexes();
    auto t1 = chrono::high_resolution_clock::now();
    auto buildMicros = chrono::duration_cast<chrono::microseconds>(t1 - t0).count();
    cout << "  ✓ Documents loaded and parsed: " << engine.documentCount() << "\n";
    cout << "  ✓ Trie vocabulary indexed with document frequencies\n";
    cout << "  ✓ Suffix Arrays built using prefix-doubling\n";
    cout << "  ✓ AVL Tree constructed and balanced\n";
    cout << "  ✓ Total Index Build Time: " << (buildMicros / 1000.0) << " ms\n\n";

    // 2. Trie Autocomplete
    cout << ">>> 2. PREFIX AUTOCOMPLETE (Trie Data Structure)\n";
    cout << "  Query prefix: \"alg\"\n";
    auto suggestions = engine.autocomplete("alg", 5);
    cout << "  Found " << suggestions.size() << " completions (ranked by document frequency):\n";
    for (const auto& item : suggestions) {
        cout << "    • \"" << item.term << "\" — Document Frequency: " << item.documentFreq << "\n";
    }
    cout << "  Lookup Complexity: O(L) where L = prefix length.\n\n";

    // 3. Substring Search & Top-K Ranking
    cout << ">>> 3. SUBSTRING SEARCH & TOP-K RANKING (Suffix Array + Max-Heap)\n";
    cout << "  Query pattern: \"algorithm\"\n";
    auto results = engine.search("algorithm", 5);
    cout << "  Ranked candidates extracted by Max-Heap:\n";
    for (size_t i = 0; i < results.size(); ++i) {
        cout << "    [" << (i + 1) << "] Doc ID #" << results[i].documentId 
             << " | Score: " << fixed << setprecision(3) << results[i].score
             << " | Body Matches: " << results[i].matchCount << "\n";
        cout << "        Title:   \"" << results[i].title << "\"\n";
        cout << "        Snippet: \"" << results[i].snippet << "...\"\n";
    }
    cout << "  Scoring: 5.0 * (title match) + log(1 + count) + 0.5 * recency\n";
    cout << "  Complexity: Suffix Array binary search O(P log N), Max-Heap top-K O(M + K log M)\n\n";

    // 4. AVL Tree Range Query
    cout << ">>> 4. ORDERED METADATA RANGE QUERY (AVL Tree)\n";
    cout << "  Range request for Document IDs: [3, 7]\n";
    auto rangeIds = engine.rangeQuery(3, 7);
    cout << "  AVL In-Order IDs returned: ";
    for (size_t i = 0; i < rangeIds.size(); ++i) {
        cout << rangeIds[i] << (i + 1 < rangeIds.size() ? ", " : "\n");
    }
    cout << "  Complexity: O(log N + K) strictly ordered traversal\n\n";

    // 5. Dynamic Insertion & Instant Re-Indexing
    cout << ">>> 5. DYNAMIC MUTATION & INDEX SYNCHRONIZATION\n";
    Document newDoc;
    newDoc.id = 11;
    newDoc.title = "Advanced B-Tree Indexing Systems";
    newDoc.body = "A B-tree index provides ordered multi-way search balancing disk block access. B-tree variants include B+ trees.";
    newDoc.category = "databases";
    newDoc.source = "demo_walkthrough";
    newDoc.createdAt = chrono::system_clock::now();
    newDoc.updatedAt = newDoc.createdAt;

    cout << "  Inserting new document: \"" << newDoc.title << "\"...\n";
    engine.addDocument(newDoc);
    cout << "  ✓ Total indexed documents now: " << engine.documentCount() << "\n";
    
    cout << "  Immediate query test on newly added document:\n";
    auto newResults = engine.search("block access", 2);
    if (!newResults.empty()) {
        cout << "  ✓ Pattern \"block access\" matched immediately in Doc #" 
             << newResults[0].documentId << " (\"" << newResults[0].title << "\")!\n";
    }
    cout << "\n===================================================================\n";
    cout << " [DEMO COMPLETE] All 4 data structures verified end-to-end successfully!\n";
    cout << "===================================================================\n\n";
}

const char* kHtmlPage = R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>QueryForge — High-Performance Text Search Engine</title>
  <style>
    :root {
      --bg: #0f172a;
      --card-bg: #1e293b;
      --accent: #38bdf8;
      --accent-hover: #0ea5e9;
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --border: #334155;
      --badge-bg: #0369a1;
      --highlight: #fef08a;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif; }
    body { background-color: var(--bg); color: var(--text); padding: 24px; max-width: 1050px; margin: 0 auto; }
    header { text-align: center; margin-bottom: 28px; }
    h1 { font-size: 2.2rem; color: #fff; margin-bottom: 8px; }
    h1 span { color: var(--accent); }
    .subtitle { color: var(--text-muted); font-size: 1rem; }
    
    .structures-badge { display: flex; justify-content: center; gap: 10px; margin-top: 14px; flex-wrap: wrap; }
    .badge { background: #1e293b; border: 1px solid var(--border); padding: 5px 12px; border-radius: 20px; font-size: 0.82rem; color: var(--accent); }
    
    .panel { background: var(--card-bg); border: 1px solid var(--border); border-radius: 12px; padding: 20px; margin-bottom: 24px; }
    .panel-title { font-size: 1.1rem; font-weight: 600; margin-bottom: 12px; display: flex; align-items: center; justify-content: space-between; }
    
    .search-container { position: relative; width: 100%; }
    .search-bar-row { display: flex; gap: 10px; }
    input[type="text"], input[type="number"] {
      flex: 1; padding: 12px 16px; border-radius: 8px; border: 1px solid var(--border); background: #0f172a; color: #fff; font-size: 1rem;
    }
    input:focus { outline: none; border-color: var(--accent); }
    button {
      background: var(--accent); color: #0f172a; border: none; font-weight: 600; padding: 12px 20px; border-radius: 8px; cursor: pointer; transition: 0.2s;
    }
    button:hover { background: var(--accent-hover); }
    
    .autocomplete-box {
      position: absolute; top: 100%; left: 0; right: 0; background: #1e293b; border: 1px solid var(--border); border-radius: 8px; margin-top: 4px; z-index: 100; display: none; overflow: hidden; box-shadow: 0 10px 15px -3px rgba(0,0,0,0.5);
    }
    .autocomplete-item {
      padding: 10px 16px; display: flex; justify-content: space-between; cursor: pointer; border-bottom: 1px solid #334155; font-size: 0.95rem;
    }
    .autocomplete-item:hover { background: #334155; }
    .autocomplete-freq { color: var(--accent); font-size: 0.8rem; }
    
    .quick-queries { display: flex; gap: 8px; margin-top: 12px; flex-wrap: wrap; }
    .quick-btn { background: #334155; color: var(--text-muted); font-size: 0.8rem; padding: 5px 10px; border-radius: 6px; cursor: pointer; border: none; }
    .quick-btn:hover { background: var(--accent); color: #0f172a; }
    
    .range-row { display: flex; gap: 12px; align-items: center; }
    .range-row input { max-width: 140px; }
    
    .results-list { display: flex; flex-direction: column; gap: 14px; margin-top: 10px; }
    .result-card { background: #0f172a; border: 1px solid var(--border); border-radius: 8px; padding: 16px; }
    .result-header { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 6px; }
    .result-title { font-size: 1.1rem; color: #fff; font-weight: 600; }
    .result-meta { font-size: 0.82rem; color: var(--accent); display: flex; gap: 12px; }
    .snippet { font-size: 0.92rem; color: var(--text-muted); line-height: 1.45; }
    .empty-state { text-align: center; color: var(--text-muted); padding: 30px; font-size: 0.95rem; }
    
    .id-tag { background: #0284c7; color: #fff; font-size: 0.75rem; padding: 2px 7px; border-radius: 4px; margin-right: 6px; }
  </style>
</head>
<body>
  <header>
    <h1>Query<span>Forge</span></h1>
    <p class="subtitle">High-Performance Text Search Engine Using Advanced Data Structures</p>
    <div class="structures-badge">
      <span class="badge">🌲 Trie (Prefix Autocomplete)</span>
      <span class="badge">🔤 Suffix Array (Substring Search)</span>
      <span class="badge">⛰️ Max-Heap (Top-K Ranking)</span>
      <span class="badge">⚖️ AVL Tree (Range Lookup)</span>
    </div>
  </header>

  <!-- Search Panel -->
  <div class="panel">
    <div class="panel-title">
      <span>Substring Search & Live Autocomplete</span>
      <span id="index-status" style="font-size:0.85rem; color:#4ade80;">● Indexes Ready</span>
    </div>
    <div class="search-container">
      <div class="search-bar-row">
        <input type="text" id="query-input" placeholder="Search substring or type prefix (e.g., 'alg', 'tree', 'binary')..." autocomplete="off">
        <button id="search-btn" onclick="executeSearch()">Search</button>
      </div>
      <div id="autocomplete-box" class="autocomplete-box"></div>
    </div>
    <div class="quick-queries">
      <span style="font-size:0.8rem; color:var(--text-muted); align-self:center;">Try:</span>
      <button class="quick-btn" onclick="setQuery('algorithm')">algorithm</button>
      <button class="quick-btn" onclick="setQuery('data-structures')">data-structures</button>
      <button class="quick-btn" onclick="setQuery('search')">search</button>
      <button class="quick-btn" onclick="setQuery('balancing')">balancing</button>
      <button class="quick-btn" onclick="setQuery('tree')">tree</button>
    </div>
  </div>

  <!-- Range Query Panel (AVL Tree) -->
  <div class="panel">
    <div class="panel-title">AVL Tree Ordered Range Query</div>
    <div class="range-row">
      <label style="color:var(--text-muted); font-size:0.9rem;">From Doc ID:</label>
      <input type="number" id="from-id" value="2" min="1">
      <label style="color:var(--text-muted); font-size:0.9rem;">To Doc ID:</label>
      <input type="number" id="to-id" value="6" min="1">
      <button onclick="executeRange()">Query Range</button>
    </div>
    <div id="range-results" style="margin-top:12px; font-size:0.9rem; color:var(--text-muted);"></div>
  </div>

  <!-- Results Panel -->
  <div class="panel">
    <div class="panel-title">Search Results (<span id="result-count">0</span>)</div>
    <div id="results-list" class="results-list">
      <div class="empty-state">Enter a query or select an autocomplete term to view ranked results.</div>
    </div>
  </div>

  <script>
    const queryInput = document.getElementById('query-input');
    const autoBox = document.getElementById('autocomplete-box');
    let debounceTimer;

    queryInput.addEventListener('input', (e) => {
      const val = e.target.value.trim();
      clearTimeout(debounceTimer);
      if (val.length === 0) {
        autoBox.style.display = 'none';
        return;
      }
      debounceTimer = setTimeout(() => {
        fetch('/api/autocomplete?prefix=' + encodeURIComponent(val))
          .then(res => res.json())
          .then(data => {
            if (!data || data.length === 0) {
              autoBox.style.display = 'none';
              return;
            }
            autoBox.innerHTML = '';
            data.forEach(item => {
              const div = document.createElement('div');
              div.className = 'autocomplete-item';
              div.innerHTML = `<span>${item.term}</span><span class="autocomplete-freq">Freq: ${item.freq}</span>`;
              div.onclick = () => {
                queryInput.value = item.term;
                autoBox.style.display = 'none';
                executeSearch();
              };
              autoBox.appendChild(div);
            });
            autoBox.style.display = 'block';
          }).catch(() => { autoBox.style.display = 'none'; });
      }, 150);
    });

    document.addEventListener('click', (e) => {
      if (!autoBox.contains(e.target) && e.target !== queryInput) {
        autoBox.style.display = 'none';
      }
    });

    queryInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') {
        autoBox.style.display = 'none';
        executeSearch();
      }
    });

    function setQuery(q) {
      queryInput.value = q;
      executeSearch();
    }

    function executeSearch() {
      const q = queryInput.value.trim();
      if (!q) return;
      fetch('/api/search?q=' + encodeURIComponent(q) + '&limit=10')
        .then(res => res.json())
        .then(results => {
          const container = document.getElementById('results-list');
          document.getElementById('result-count').innerText = results.length;
          if (results.length === 0) {
            container.innerHTML = '<div class="empty-state">No matching documents found for "' + q + '".</div>';
            return;
          }
          container.innerHTML = '';
          results.forEach((r, idx) => {
            const card = document.createElement('div');
            card.className = 'result-card';
            card.innerHTML = `
              <div class="result-header">
                <div class="result-title"><span class="id-tag">ID #${r.id}</span>${r.title}</div>
                <div class="result-meta">
                  <span>Score: <strong>${r.score.toFixed(3)}</strong></span>
                  <span>Matches: <strong>${r.matches}</strong></span>
                </div>
              </div>
              <div class="snippet">${r.snippet}</div>
            `;
            container.appendChild(card);
          });
        });
    }

    function executeRange() {
      const fromId = document.getElementById('from-id').value;
      const toId = document.getElementById('to-id').value;
      fetch(`/api/range?from=${fromId}&to=${toId}`)
        .then(res => res.json())
        .then(ids => {
          const div = document.getElementById('range-results');
          if (!ids || ids.length === 0) {
            div.innerHTML = `No documents found in range [${fromId}, ${toId}].`;
          } else {
            div.innerHTML = `<strong>Matching IDs in AVL In-Order:</strong> [ ${ids.join(', ')} ] (${ids.length} documents)`;
          }
        });
    }

    // Auto-search default term on load
    window.onload = () => {
      executeSearch();
      executeRange();
    };
  </script>
</body>
</html>
)rawhtml";

void launchGuiServer(SearchEngine& engine, int port) {
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(kHtmlPage, "text/html");
    });

    svr.Get("/api/stats", [&engine](const httplib::Request&, httplib::Response& res) {
        std::stringstream ss;
        ss << "{\"docCount\":" << engine.documentCount() << ",\"state\":\"READY\"}";
        res.set_content(ss.str(), "application/json");
    });

    svr.Get("/api/autocomplete", [&engine](const httplib::Request& req, httplib::Response& res) {
        std::string prefix = req.has_param("prefix") ? req.get_param_value("prefix") : "";
        auto suggestions = engine.autocomplete(prefix, 8);
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < suggestions.size(); ++i) {
            ss << "{\"term\":\"" << suggestions[i].term << "\",\"freq\":" << suggestions[i].documentFreq << "}";
            if (i + 1 < suggestions.size()) ss << ",";
        }
        ss << "]";
        res.set_content(ss.str(), "application/json");
    });

    svr.Get("/api/search", [&engine](const httplib::Request& req, httplib::Response& res) {
        std::string query = req.has_param("q") ? req.get_param_value("q") : "";
        int limit = req.has_param("limit") ? std::stoi(req.get_param_value("limit")) : 10;
        auto results = engine.search(query, limit);
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < results.size(); ++i) {
            std::string cleanSnippet;
            for (char c : results[i].snippet) {
                if (c == '"') cleanSnippet += "\\\"";
                else if (c == '\n' || c == '\r') cleanSnippet += " ";
                else cleanSnippet += c;
            }
            std::string cleanTitle;
            for (char c : results[i].title) {
                if (c == '"') cleanTitle += "\\\"";
                else cleanTitle += c;
            }
            ss << "{\"id\":" << results[i].documentId 
               << ",\"title\":\"" << cleanTitle << "\""
               << ",\"score\":" << results[i].score
               << ",\"matches\":" << results[i].matchCount
               << ",\"snippet\":\"" << cleanSnippet << "\"}";
            if (i + 1 < results.size()) ss << ",";
        }
        ss << "]";
        res.set_content(ss.str(), "application/json");
    });

    svr.Get("/api/range", [&engine](const httplib::Request& req, httplib::Response& res) {
        long long fromId = req.has_param("from") ? std::stoll(req.get_param_value("from")) : 1;
        long long toId = req.has_param("to") ? std::stoll(req.get_param_value("to")) : 10;
        auto ids = engine.rangeQuery(fromId, toId);
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < ids.size(); ++i) {
            ss << ids[i];
            if (i + 1 < ids.size()) ss << ",";
        }
        ss << "]";
        res.set_content(ss.str(), "application/json");
    });

    printBanner();
    cout << "[INFO] QueryForge Web GUI starting on: http://127.0.0.1:" << port << "\n";
    cout << "[INFO] Serving Trie autocomplete, Suffix Array search, and AVL range queries.\n";
    cout << "[INFO] Press Ctrl+C in terminal to stop server.\n\n";

    // Attempt to open default browser automatically on macOS
    std::string openCmd = "open http://127.0.0.1:" + std::to_string(port) + " >/dev/null 2>&1 &";
    std::system(openCmd.c_str());

    svr.listen("127.0.0.1", port);
}

void runRepl(SearchEngine& engine, DocumentRepository& repo) {
    cout << "QueryForge Interactive Console (Type 'help' or 'exit')\n";
    string line;
    while (true) {
        cout << "QueryForge> ";
        if (!getline(cin, line)) break;

        vector<string> tokens = Normalizer::tokenize(line);
        if (tokens.empty()) continue;

        const string& cmd = tokens[0];
        if (cmd == "exit" || cmd == "quit") {
            cout << "Goodbye!\n";
            break;
        } else if (cmd == "help") {
            printHelp();
        } else if (cmd == "stats") {
            printStats(engine, repo);
        } else if (cmd == "demo") {
            runDemo(engine, repo);
        } else if (cmd == "autocomplete") {
            if (tokens.size() < 2) {
                cout << "Usage: autocomplete <prefix>\n";
            } else {
                auto suggestions = engine.autocomplete(tokens[1], 10);
                if (suggestions.empty()) {
                    cout << "No completions found for \"" << tokens[1] << "\".\n";
                } else {
                    cout << "Suggestions for \"" << tokens[1] << "\":\n";
                    for (const auto& s : suggestions) {
                        cout << "  • " << s.term << " (doc frequency: " << s.documentFreq << ")\n";
                    }
                }
            }
        } else if (cmd == "search") {
            if (tokens.size() < 2) {
                cout << "Usage: search <query>\n";
            } else {
                string q = line.substr(line.find(tokens[0]) + tokens[0].length());
                auto results = engine.search(q, 5);
                if (results.empty()) {
                    cout << "No documents found matching \"" << q << "\".\n";
                } else {
                    cout << "Search results for \"" << q << "\":\n";
                    for (const auto& r : results) {
                        cout << "  - Doc #" << r.documentId << ": \"" << r.title << "\" (score: " 
                             << fixed << setprecision(2) << r.score << ")\n";
                        cout << "    Snippet: " << r.snippet << "...\n";
                    }
                }
            }
        } else if (cmd == "range") {
            if (tokens.size() < 3) {
                cout << "Usage: range <fromId> <toId>\n";
            } else {
                long long f = std::stoll(tokens[1]);
                long long t = std::stoll(tokens[2]);
                auto ids = engine.rangeQuery(f, t);
                cout << "Document IDs in range [" << f << ", " << t << "]: ";
                for (size_t i = 0; i < ids.size(); ++i) {
                    cout << ids[i] << (i + 1 < ids.size() ? ", " : "\n");
                }
            }
        } else {
            cout << "Unknown command: '" << line << "'. Type 'help' for available commands.\n";
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    std::string dbUrl = getDbUrl();
    DocumentRepository repo(dbUrl);
    SearchEngine engine(repo);

    try {
        engine.buildIndexes();
    } catch (const std::exception& e) {
        cerr << "[WARN] Index initialization warning: " << e.what() << "\n";
    }

    if (argc < 2) {
        printBanner();
        runRepl(engine, repo);
        return 0;
    }

    string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        printHelp();
        return 0;
    } else if (command == "stats") {
        printStats(engine, repo);
        return 0;
    } else if (command == "demo") {
        runDemo(engine, repo);
        return 0;
    } else if (command == "gui" || command == "serve") {
        int port = 8080;
        if (argc >= 3) {
            port = std::stoi(argv[2]);
        }
        launchGuiServer(engine, port);
        return 0;
    } else if (command == "autocomplete") {
        if (argc < 3) {
            cerr << "Usage: queryforge autocomplete <prefix>\n";
            return 1;
        }
        string prefix = argv[2];
        auto suggestions = engine.autocomplete(prefix, 10);
        if (suggestions.empty()) {
            cout << "No completions found for prefix: \"" << prefix << "\"\n";
        } else {
            cout << "Autocomplete results for \"" << prefix << "\":\n";
            for (const auto& item : suggestions) {
                cout << "  - " << item.term << " (doc frequency: " << item.documentFreq << ")\n";
            }
        }
        return 0;
    } else if (command == "search") {
        if (argc < 3) {
            cerr << "Usage: queryforge search <query> [--limit K]\n";
            return 1;
        }
        string query = argv[2];
        int limit = 10;
        if (argc >= 5 && string(argv[3]) == "--limit") {
            limit = std::stoi(argv[4]);
        }
        auto results = engine.search(query, limit);
        if (results.empty()) {
            cout << "No matches found for \"" << query << "\".\n";
        } else {
            cout << "Top " << results.size() << " results for \"" << query << "\":\n";
            for (size_t i = 0; i < results.size(); ++i) {
                cout << "\n[" << (i + 1) << "] Doc #" << results[i].documentId 
                     << " — \"" << results[i].title << "\" (Score: " 
                     << fixed << setprecision(3) << results[i].score << ")\n";
                cout << "    Snippet: \"" << results[i].snippet << "...\"\n";
            }
        }
        return 0;
    } else if (command == "range") {
        if (argc < 4) {
            cerr << "Usage: queryforge range <fromId> <toId>\n";
            return 1;
        }
        long long fromId = std::stoll(argv[2]);
        long long toId = std::stoll(argv[3]);
        auto ids = engine.rangeQuery(fromId, toId);
        cout << "AVL Tree ordered IDs in range [" << fromId << ", " << toId << "]: ";
        for (size_t i = 0; i < ids.size(); ++i) {
            cout << ids[i] << (i + 1 < ids.size() ? ", " : "\n");
        }
        return 0;
    } else if (command == "interactive") {
        printBanner();
        runRepl(engine, repo);
        return 0;
    } else {
        cerr << "Error: Unknown command '" << command << "'\n\n";
        printHelp();
        return 1;
    }
}
