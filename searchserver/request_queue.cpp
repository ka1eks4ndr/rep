#include "request_queue.h"
#include "document.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) 
: s(search_server)
{   
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    auto result=s.FindTopDocuments(raw_query,status);
    requests_.push_back({result,result.empty()});
    if (requests_.size()>sec_in_day_) {
        requests_.pop_front();
    }
    return result;
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    auto result=s.FindTopDocuments(raw_query);
    requests_.push_back({result,result.empty()});
    if (requests_.size()>sec_in_day_) {
        requests_.pop_front();
    }
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return count_if(begin(requests_),end(requests_),[](QueryResult queryresult ){return queryresult.empty==true;});
}
