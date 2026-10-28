#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <chrono>
#include "models/document.hpp"
#include "models/search_result.hpp"
#include "models/search_history_entry.hpp"
#include "engine/normalizer.hpp"
#include "structures/trie.hpp"

using namespace std;

namespace {

void printHelp() {
    cout << "QueryForge — High-Performance Text Search Engine\n"
         << "Usage: queryforge <command> [arguments]\n\n"
         << "Commands:\n"
         << "  help                      Show this help message\n"
         << "  stats                     Display index and system statistics\n"
         << "  autocomplete <prefix>     Suggest terms matching prefix\n"
         << "  search <query> [--limit K] Run substring search and return top-K results\n"
         << "  range <fromId> <toId>     Perform AVL range query for document IDs\n"
         << "  interactive               Start interactive CLI REPL session\n\n";
}

void printStats(const Trie& trie) {
    cout << "--- QueryForge Statistics ---\n"
         << "Index Status: READY (Trie / Normalizer standalone mode)\n"
         << "Trie Unique Vocabulary Terms: " << trie.uniqueTermCount() << "\n"
         << "Supported Features: Prefix Autocomplete, Normalization\n"
         << "Connected Database: None (or QUERYFORGE_DB_URL pending)\n"
         << "-----------------------------\n";
}

void runRepl(Trie& trie) {
    cout << "QueryForge Interactive Console (Type 'help' or 'exit')\n";
    string line;
    while (true) {
        cout << "QueryForge> ";
        if (!getline(cin, line)) {
            break;
        }

        vector<string> tokens = Normalizer::tokenize(line);
        if (tokens.empty()) {
            continue;
        }

        const string& cmd = tokens[0];
        if (cmd == "exit" || cmd == "quit") {
            cout << "Goodbye!\n";
            break;
        } else if (cmd == "help") {
            printHelp();
        } else if (cmd == "stats") {
            printStats(trie);
        } else if (cmd == "autocomplete") {
            if (tokens.size() < 2) {
                cout << "Usage: autocomplete <prefix>\n";
            } else {
                auto suggestions = trie.autocomplete(tokens[1], 10);
                if (suggestions.empty()) {
                    cout << "No completions found for \"" << tokens[1] << "\".\n";
                } else {
                    cout << "Suggestions for \"" << tokens[1] << "\":\n";
                    for (const auto& s : suggestions) {
                        cout << "  • " << s.term << " (doc frequency: " << s.documentFreq << ")\n";
                    }
                }
            }
        } else {
            cout << "Unknown command: '" << line << "'. Type 'help' for available commands.\n";
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    Trie sampleTrie;
    // Pre-populate standard sample vocabulary for immediate utility
    sampleTrie.insertTerm("algorithm", 1);
    sampleTrie.insertTerm("algorithmic", 1);
    sampleTrie.insertTerm("algorithms", 2);
    sampleTrie.insertTerm("avl", 1);
    sampleTrie.insertTerm("binary", 1);
    sampleTrie.insertTerm("data", 1);
    sampleTrie.insertTerm("database", 2);
    sampleTrie.insertTerm("datastructure", 3);
    sampleTrie.insertTerm("heap", 1);
    sampleTrie.insertTerm("indexing", 1);
    sampleTrie.insertTerm("normalizer", 1);
    sampleTrie.insertTerm("queryforge", 1);
    sampleTrie.insertTerm("search", 1);
    sampleTrie.insertTerm("suffix", 1);
    sampleTrie.insertTerm("trie", 1);

    if (argc < 2) {
        runRepl(sampleTrie);
        return 0;
    }

    string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        printHelp();
        return 0;
    } else if (command == "stats") {
        printStats(sampleTrie);
        return 0;
    } else if (command == "autocomplete") {
        if (argc < 3) {
            cerr << "Error: 'autocomplete' requires a prefix argument.\n"
                 << "Usage: queryforge autocomplete <prefix>\n";
            return 1;
        }
        string prefix = Normalizer::normalizeToken(argv[2]);
        auto suggestions = sampleTrie.autocomplete(prefix, 10);
        if (suggestions.empty()) {
            cout << "No completions found for prefix: \"" << prefix << "\"\n";
        } else {
            cout << "Autocomplete results for \"" << prefix << "\":\n";
            for (const auto& item : suggestions) {
                cout << "  - " << item.term << " (freq: " << item.documentFreq << ")\n";
            }
        }
        return 0;
    } else if (command == "interactive") {
        runRepl(sampleTrie);
        return 0;
    } else if (command == "search" || command == "range") {
        cout << "[INFO] Command '" << command << "' acknowledged. Integration with SearchEngine will be active upon Checkpoint 2/3 merge.\n";
        return 0;
    } else {
        cerr << "Error: Unknown command '" << command << "'\n\n";
        printHelp();
        return 1;
    }
}
