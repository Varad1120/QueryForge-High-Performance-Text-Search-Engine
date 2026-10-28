#pragma once
#include <string>
#include <chrono>

using namespace std;

struct SearchHistoryEntry {
    string    queryText;
    string    queryType;        // "substring" | "autocomplete" | "range"
    int       requestedLimit{10};
    int       resultCount{0};
    long long elapsedMicros{0};
};
