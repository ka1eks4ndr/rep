#include "remove_duplicates.h"
#include "search_server.h"

#include <iostream>

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    vector <int> for_delete;
    set<map<string_view,double>> filter;
    for (const auto document_id : search_server) {
        auto words = search_server.GetWordFrequencies(document_id);
        for (auto& [word,frequencies] : words ) {
            frequencies=0;
        }
        auto [element,unique] = filter.emplace(words);
        if (!unique) {
            for_delete.push_back(document_id);    
        }
    }
    for (const auto element : for_delete) {
        cout<<"Found duplicate document id "<<element<<endl;
        search_server.RemoveDocument(element);
    }
}