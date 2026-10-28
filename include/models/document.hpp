#pragma once
#include <string>
#include <chrono>

using namespace std;

struct Document {
    long long id{0};
    string    title;
    string    body;
    string    category;
    string    source;           // optional, may be empty
    chrono::system_clock::time_point createdAt;
    chrono::system_clock::time_point updatedAt;
};
