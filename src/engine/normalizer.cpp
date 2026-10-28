#include "engine/normalizer.hpp"
#include <cctype>

using namespace std;

vector<string> Normalizer::tokenize(const string& text) {
    vector<string> tokens;
    string current;
    for (char c : text) {
        if (isalnum(static_cast<unsigned char>(c))) {
            current.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
        } else {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

string Normalizer::normalizeToken(const string& token) {
    string result;
    result.reserve(token.size());
    for (char c : token) {
        if (isalnum(static_cast<unsigned char>(c))) {
            result.push_back(static_cast<char>(tolower(static_cast<unsigned char>(c))));
        }
    }
    return result;
}
