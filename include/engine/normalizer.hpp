#pragma once
#include <string>
#include <vector>

using namespace std;

class Normalizer {
public:
    // Convert text to lowercase; strip punctuation; split on whitespace.
    // Returns a list of tokens. Empty input returns an empty vector.
    static vector<string> tokenize(const string& text);

    // Normalize a single string to lowercase; used for query normalization.
    static string normalizeToken(const string& token);
};
