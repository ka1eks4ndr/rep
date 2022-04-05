#include "string_processing.h"

#include <iostream>

using namespace std;

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

vector<string_view> SplitIntoWordsView(string_view str) {
    vector<string_view> result;
    const int64_t pos_end = str.npos;
    while (true) {
        int64_t space = str.find(' ');
        result.push_back(str.substr(0, space));
        if (space == pos_end) {
            break;
        }
        else {
            ++space;
            str.remove_prefix(space);
        }
    }
    return result;
}