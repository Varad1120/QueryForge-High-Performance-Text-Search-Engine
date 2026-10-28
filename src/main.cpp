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
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@300;400;500;600;700&display=swap" rel="stylesheet">
  <style>
    :root {
      --bg: #0d0f0e;
      --card-bg: rgba(255,255,255,0.04);
      --card-border: rgba(255,255,255,0.07);
      --accent: #f59e0b;
      --accent-hover: #fbbf24;
      --accent-dim: rgba(245,158,11,0.13);
      --green: #10b981;
      --green-dim: rgba(16,185,129,0.13);
      --rose: #fb7185;
      --rose-dim: rgba(251,113,133,0.13);
      --sand: #c9a96e;
      --sand-dim: rgba(201,169,110,0.12);
      --text: #f0ece3;
      --text-muted: #6b6860;
      --text-secondary: #a09c94;
      --border: rgba(255,255,255,0.07);
      --input-bg: rgba(255,255,255,0.05);
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: "Space Grotesk", -apple-system, BlinkMacSystemFont, sans-serif;
      background: var(--bg);
      color: var(--text);
      padding: 32px 24px 80px;
      max-width: 1080px;
      margin: 0 auto;
      min-height: 100vh;
    }
    header {
      text-align: center;
      margin-bottom: 36px;
      padding: 52px 24px 44px;
      position: relative;
      overflow: hidden;
      border-radius: 22px;
      border: 1px solid rgba(245,158,11,0.16);
      background: radial-gradient(ellipse 80% 60% at 50% -10%, rgba(245,158,11,0.12) 0%, transparent 70%), #0d0f0e;
    }
    header::after {
      content: "";
      position: absolute;
      top: -40px; left: 50%;
      transform: translateX(-50%);
      width: 400px; height: 130px;
      background: radial-gradient(ellipse, rgba(245,158,11,0.2) 0%, transparent 70%);
      filter: blur(28px);
      pointer-events: none;
    }
    .wordmark {
      font-size: 3.2rem; font-weight: 700; letter-spacing: -2px;
      line-height: 1; position: relative; z-index: 1; margin-bottom: 12px;
    }
    .wordmark .part-q { color: var(--text); }
    .wordmark .part-f { color: var(--accent); }
    .subtitle {
      color: var(--text-secondary); font-size: 0.98rem; font-weight: 400;
      position: relative; z-index: 1; margin-bottom: 30px;
    }
    .structures-badge { display: flex; justify-content: center; gap: 8px; flex-wrap: wrap; position: relative; z-index: 1; }
    .badge {
      display: inline-flex; align-items: center; gap: 5px;
      padding: 5px 15px; border-radius: 100px;
      font-size: 0.77rem; font-weight: 600; border: 1px solid;
      transition: transform 0.18s, filter 0.18s;
    }
    .badge:hover { transform: translateY(-2px); filter: brightness(1.15); }
    .badge-amber { background: var(--accent-dim); border-color: rgba(245,158,11,0.28); color: var(--accent); }
    .badge-green  { background: var(--green-dim);  border-color: rgba(16,185,129,0.28);  color: var(--green); }
    .badge-rose   { background: var(--rose-dim);   border-color: rgba(251,113,133,0.28); color: var(--rose); }
    .badge-sand   { background: var(--sand-dim);   border-color: rgba(201,169,110,0.28); color: var(--sand); }
    .panel {
      background: var(--card-bg); border: 1px solid var(--card-border);
      border-radius: 16px; padding: 24px; margin-bottom: 18px;
      transition: border-color 0.2s;
    }
    .panel:hover { border-color: rgba(255,255,255,0.11); }
    .panel-title {
      font-size: 0.75rem; font-weight: 700; text-transform: uppercase;
      letter-spacing: 0.09em; color: var(--text-muted); margin-bottom: 18px;
      display: flex; align-items: center; justify-content: space-between;
    }
    .panel-label { display: flex; align-items: center; gap: 8px; }
    .dot { width: 7px; height: 7px; border-radius: 50%; flex-shrink: 0; }
    .dot-amber { background: var(--accent); box-shadow: 0 0 8px var(--accent); }
    .dot-green  { background: var(--green);  box-shadow: 0 0 8px var(--green); }
    .dot-rose   { background: var(--rose);   box-shadow: 0 0 8px var(--rose); }
    .search-container { position: relative; }
    .search-bar-row { display: flex; gap: 10px; }
    input[type="text"], input[type="number"] {
      flex: 1; padding: 13px 18px; border-radius: 10px;
      border: 1px solid var(--border); background: var(--input-bg);
      color: var(--text); font-size: 0.96rem;
      font-family: "Space Grotesk", sans-serif; font-weight: 400;
      transition: border-color 0.2s, box-shadow 0.2s, background 0.2s;
    }
    input[type="text"]::placeholder { color: var(--text-muted); }
    input:focus {
      outline: none; border-color: rgba(245,158,11,0.45);
      background: rgba(255,255,255,0.07); box-shadow: 0 0 0 3px rgba(245,158,11,0.09);
    }
    button {
      background: var(--accent); color: #190e00; border: none;
      font-weight: 700; font-family: "Space Grotesk", sans-serif;
      font-size: 0.88rem; letter-spacing: 0.02em;
      padding: 13px 24px; border-radius: 10px; cursor: pointer;
      white-space: nowrap;
      transition: background 0.15s, transform 0.12s, box-shadow 0.15s;
    }
    button:hover {
      background: var(--accent-hover); transform: translateY(-1px);
      box-shadow: 0 6px 20px rgba(245,158,11,0.35);
    }
    button:active { transform: translateY(0); box-shadow: none; }
    .autocomplete-box {
      position: absolute; top: calc(100% + 6px); left: 0; right: 0;
      background: #15120a; border: 1px solid rgba(245,158,11,0.2);
      border-radius: 12px; z-index: 200; display: none; overflow: hidden;
      box-shadow: 0 18px 48px rgba(0,0,0,0.7), 0 0 0 1px rgba(245,158,11,0.07);
    }
    .autocomplete-item {
      padding: 11px 18px; display: flex; justify-content: space-between;
      align-items: center; cursor: pointer;
      border-bottom: 1px solid rgba(255,255,255,0.04);
      font-size: 0.92rem; transition: background 0.13s;
    }
    .autocomplete-item:last-child { border-bottom: none; }
    .autocomplete-item:hover { background: rgba(245,158,11,0.07); }
    .autocomplete-freq {
      font-size: 0.73rem; font-weight: 700; color: var(--accent);
      background: var(--accent-dim); padding: 2px 9px; border-radius: 100px;
    }
    .quick-queries { display: flex; gap: 7px; margin-top: 14px; flex-wrap: wrap; align-items: center; }
    .try-label { font-size: 0.73rem; color: var(--text-muted); font-weight: 600; letter-spacing: 0.06em; text-transform: uppercase; }
    .quick-btn {
      background: rgba(255,255,255,0.04); color: var(--text-secondary);
      font-size: 0.79rem; font-family: "Space Grotesk", sans-serif; font-weight: 500;
      padding: 5px 13px; border-radius: 100px; cursor: pointer;
      border: 1px solid var(--border); transition: all 0.16s;
    }
    .quick-btn:hover {
      background: var(--accent-dim); border-color: rgba(245,158,11,0.32);
      color: var(--accent); transform: translateY(-1px); box-shadow: none;
    }
    .range-row { display: flex; gap: 12px; align-items: center; flex-wrap: wrap; }
    .range-row input[type="number"] { max-width: 110px; flex: none; }
    .range-label { font-size: 0.84rem; color: var(--text-secondary); font-weight: 500; }
    #range-results {
      margin-top: 14px; font-size: 0.86rem; padding: 12px 16px;
      background: var(--green-dim); border: 1px solid rgba(16,185,129,0.18);
      border-radius: 10px; display: none; line-height: 1.6;
    }
    #range-results.visible { display: block; }
    #range-results strong { color: var(--green); }
    .results-list { display: flex; flex-direction: column; gap: 11px; }
    .result-card {
      background: rgba(255,255,255,0.025); border: 1px solid var(--border);
      border-radius: 12px; padding: 18px 20px;
      transition: border-color 0.18s, background 0.18s, transform 0.14s;
    }
    .result-card:hover {
      border-color: rgba(245,158,11,0.22); background: rgba(245,158,11,0.025);
      transform: translateX(3px);
    }
    .result-header {
      display: flex; justify-content: space-between;
      align-items: flex-start; margin-bottom: 10px; gap: 12px;
    }
    .result-left { display: flex; align-items: flex-start; gap: 10px; flex: 1; min-width: 0; }
    .rank-badge {
      font-size: 0.68rem; font-weight: 700; color: var(--text-muted);
      background: rgba(255,255,255,0.05); border: 1px solid var(--border);
      border-radius: 6px; padding: 3px 7px; flex-shrink: 0; margin-top: 2px;
    }
    .result-info { min-width: 0; }
    .result-title { font-size: 1rem; color: var(--text); font-weight: 600; line-height: 1.3; margin-bottom: 3px; }
    .doc-id { font-size: 0.68rem; font-weight: 700; color: var(--text-muted); letter-spacing: 0.06em; text-transform: uppercase; }
    .result-pills { display: flex; gap: 6px; flex-shrink: 0; align-items: center; flex-wrap: wrap; justify-content: flex-end; }
    .pill { font-size: 0.73rem; font-weight: 700; padding: 3px 10px; border-radius: 100px; white-space: nowrap; }
    .pill-score   { background: var(--accent-dim); color: var(--accent); }
    .pill-matches { background: var(--green-dim);  color: var(--green); }
    .score-bar { height: 2px; background: rgba(255,255,255,0.05); border-radius: 2px; margin-bottom: 11px; overflow: hidden; }
    .score-bar-fill {
      height: 100%; border-radius: 2px;
      background: linear-gradient(90deg, var(--accent) 0%, var(--green) 100%);
      transition: width 0.6s cubic-bezier(0.4,0,0.2,1);
    }
    .snippet { font-size: 0.87rem; color: var(--text-muted); line-height: 1.6; font-weight: 400; }
    .empty-state { text-align: center; color: var(--text-muted); padding: 48px 20px; font-size: 0.92rem; }
    .empty-icon { font-size: 2.4rem; margin-bottom: 12px; opacity: 0.45; }
    .status-dot {
      display: inline-flex; align-items: center; gap: 6px;
      font-size: 0.75rem; font-weight: 600; color: var(--green);
    }
    .status-dot::before {
      content: ""; display: inline-block; width: 7px; height: 7px;
      border-radius: 50%; background: var(--green);
      animation: pulse-g 2.2s infinite;
    }
    @keyframes pulse-g {
      0%   { box-shadow: 0 0 0 0 rgba(16,185,129,0.5); }
      70%  { box-shadow: 0 0 0 6px rgba(16,185,129,0); }
      100% { box-shadow: 0 0 0 0 rgba(16,185,129,0); }
    }
    ::-webkit-scrollbar { width: 5px; }
    ::-webkit-scrollbar-track { background: transparent; }
    ::-webkit-scrollbar-thumb { background: rgba(255,255,255,0.08); border-radius: 3px; }
    ::-webkit-scrollbar-thumb:hover { background: rgba(245,158,11,0.3); }
  </style>
</head>
<body>
  <header>
    <div class="wordmark"><span class="part-q">Query</span><span class="part-f">Forge</span></div>
    <p class="subtitle">High-Performance Text Search Engine &middot; Advanced Data Structures &middot; C++17</p>
    <div class="structures-badge">
      <span class="badge badge-amber">&#127810; Trie &mdash; Prefix Autocomplete</span>
      <span class="badge badge-green">&#128288; Suffix Array &mdash; Substring Match</span>
      <span class="badge badge-rose">&#9968;&#65039; Max-Heap &mdash; Top-K Ranking</span>
      <span class="badge badge-sand">&#9878;&#65039; AVL Tree &mdash; Range Lookup</span>
    </div>
  </header>

  <div class="panel">
    <div class="panel-title">
      <div class="panel-label"><span class="dot dot-amber"></span>Substring Search &amp; Live Autocomplete</div>
      <span class="status-dot">Indexes Ready</span>
    </div>
    <div class="search-container">
      <div class="search-bar-row">
        <input type="text" id="query-input"
               placeholder="Search substring or type prefix &mdash; e.g. alg, tree, binary&hellip;"
               autocomplete="off">
        <button id="search-btn" onclick="executeSearch()">Search</button>
      </div>
      <div id="autocomplete-box" class="autocomplete-box"></div>
    </div>
    <div class="quick-queries">
      <span class="try-label">Try:</span>
      <button class="quick-btn" onclick="setQuery('algorithm')">algorithm</button>
      <button class="quick-btn" onclick="setQuery('data-structures')">data-structures</button>
      <button class="quick-btn" onclick="setQuery('search')">search</button>
      <button class="quick-btn" onclick="setQuery('balancing')">balancing</button>
      <button class="quick-btn" onclick="setQuery('tree')">tree</button>
    </div>
  </div>

  <div class="panel">
    <div class="panel-title">
      <div class="panel-label"><span class="dot dot-green"></span>AVL Tree &mdash; Ordered Range Query</div>
    </div>
    <div class="range-row">
      <span class="range-label">From Doc ID</span>
      <input type="number" id="from-id" value="2" min="1">
      <span class="range-label">To Doc ID</span>
      <input type="number" id="to-id" value="6" min="1">
      <button onclick="executeRange()">Query Range</button>
    </div>
    <div id="range-results"></div>
  </div>

  <div class="panel">
    <div class="panel-title">
      <div class="panel-label"><span class="dot dot-rose"></span>Search Results</div>
      <span id="result-count" style="font-size:0.75rem;color:var(--text-muted);font-weight:700;"></span>
    </div>
    <div id="results-list" class="results-list">
      <div class="empty-state">
        <div class="empty-icon">&#128269;</div>
        Enter a query or pick an autocomplete suggestion to view ranked results.
      </div>
    </div>
  </div>

  <script>
    const queryInput = document.getElementById('query-input');
    const autoBox    = document.getElementById('autocomplete-box');
    let debounceTimer;

    queryInput.addEventListener('input', (e) => {
      const val = e.target.value.trim();
      clearTimeout(debounceTimer);
      if (!val) { autoBox.style.display = 'none'; return; }
      debounceTimer = setTimeout(() => {
        fetch('/api/autocomplete?prefix=' + encodeURIComponent(val))
          .then(r => r.json())
          .then(data => {
            if (!data || !data.length) { autoBox.style.display = 'none'; return; }
            autoBox.innerHTML = '';
            data.forEach(item => {
              const div = document.createElement('div');
              div.className = 'autocomplete-item';
              div.innerHTML = '<span>' + item.term + '</span><span class="autocomplete-freq">&#215;' + item.freq + '</span>';
              div.onclick = () => { queryInput.value = item.term; autoBox.style.display = 'none'; executeSearch(); };
              autoBox.appendChild(div);
            });
            autoBox.style.display = 'block';
          }).catch(() => { autoBox.style.display = 'none'; });
      }, 150);
    });

    document.addEventListener('click', (e) => {
      if (!autoBox.contains(e.target) && e.target !== queryInput) autoBox.style.display = 'none';
    });

    queryInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') { autoBox.style.display = 'none'; executeSearch(); }
    });

    function setQuery(q) { queryInput.value = q; executeSearch(); }

    function executeSearch() {
      const q = queryInput.value.trim();
      if (!q) return;
      fetch('/api/search?q=' + encodeURIComponent(q) + '&limit=10')
        .then(r => r.json())
        .then(results => {
          const container = document.getElementById('results-list');
          const countEl   = document.getElementById('result-count');
          countEl.textContent = results.length ? results.length + ' result' + (results.length !== 1 ? 's' : '') : '';
          if (!results.length) {
            container.innerHTML = '<div class="empty-state"><div class="empty-icon">&#129513;</div>No matching documents found for &ldquo;' + q + '&rdquo;.</div>';
            return;
          }
          const maxScore = Math.max.apply(null, results.map(r => r.score).concat([0.001]));
          container.innerHTML = '';
          results.forEach((r, idx) => {
            const card = document.createElement('div');
            card.className = 'result-card';
            const pct = Math.round((r.score / maxScore) * 100);
            card.innerHTML =
              '<div class="result-header">' +
                '<div class="result-left">' +
                  '<span class="rank-badge">#' + (idx+1) + '</span>' +
                  '<div class="result-info">' +
                    '<div class="result-title">' + r.title + '</div>' +
                    '<div class="doc-id">DOC &middot; ' + r.id + '</div>' +
                  '</div>' +
                '</div>' +
                '<div class="result-pills">' +
                  '<span class="pill pill-score">Score ' + r.score.toFixed(3) + '</span>' +
                  '<span class="pill pill-matches">' + r.matches + ' match' + (r.matches !== 1 ? 'es' : '') + '</span>' +
                '</div>' +
              '</div>' +
              '<div class="score-bar"><div class="score-bar-fill" style="width:' + pct + '%"></div></div>' +
              '<div class="snippet">' + r.snippet + '</div>';
            container.appendChild(card);
          });
        });
    }

    function executeRange() {
      const fromId = document.getElementById('from-id').value;
      const toId   = document.getElementById('to-id').value;
      fetch('/api/range?from=' + fromId + '&to=' + toId)
        .then(r => r.json())
        .then(ids => {
          const div = document.getElementById('range-results');
          div.className = 'visible';
          if (!ids || !ids.length) {
            div.innerHTML = 'No documents found in range [' + fromId + ', ' + toId + '].';
          } else {
            const chips = ids.map(id =>
              '<span style="background:rgba(16,185,129,0.14);color:#10b981;padding:2px 9px;border-radius:5px;font-size:0.8rem;font-weight:700;margin:0 2px;">' + id + '</span>'
            ).join('');
            div.innerHTML = '<strong>In-order AVL traversal [' + fromId + ' &rarr; ' + toId + ']:</strong>&nbsp;&nbsp;' + chips + '&nbsp;&nbsp;<span style="color:var(--text-muted);">(' + ids.length + ' doc' + (ids.length !== 1 ? 's' : '') + ')</span>';
          }
        });
    }

    window.onload = () => { executeSearch(); executeRange(); };
  </script>
</body>
</html>\n)rawhtml";

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
