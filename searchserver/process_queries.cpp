#include "process_queries.h"
#include <numeric>
#include <execution>
#include <string>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
        std::vector<std::vector<Document>> result(queries.size());
        std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(),
            [&search_server](const std::string& query) { return search_server.FindTopDocuments(query);});
        return result;
    }
    
std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
        auto vv=ProcessQueries(search_server,queries);
        std::list<Document> result;
        for (const auto& v : vv) {
            for (const auto& d : v) {
                result.push_back(d);
            }
        }
    return result;
    }     