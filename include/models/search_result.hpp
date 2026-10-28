#pragma once
#include <string>

using namespace std;

struct SearchResult {
    long long documentId{0};
    string    title;
    string    snippet;          // 150–200 char excerpt around first match
    double    score{0.0};
    int       matchCount{0};
};
