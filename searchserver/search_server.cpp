#include "log_duration.h"
#include "search_server.h"

#include <cmath>

using namespace std;

SearchServer::SearchServer(const string& stop_words_text)
    : SearchServer::SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor
                                                        // from string container
{
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
        throw invalid_argument("Invalid document_id"s);
    }
    string str;
    for (const auto& c : document) {
        str.push_back(c);
    }
    documents_strings_[document_id]=str;
    const auto words = SplitIntoWordViewsNoStop(documents_strings_[document_id]);

    const double inv_word_count = 1.0 / words.size();
    for (const auto& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_word_freq_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.emplace(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments( std::execution::sequenced_policy seq, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(seq,raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments( std::execution::parallel_policy par, std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(par,raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

vector<Document> SearchServer::FindTopDocuments( string_view raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments( std::execution::sequenced_policy seq, std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(seq, raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments( std::execution::parallel_policy par, std::string_view raw_query) const {
    return SearchServer::FindTopDocuments(par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::iterator SearchServer::begin() {
    return document_ids_.begin();
}

std::set<int>::iterator SearchServer::end () {
    return document_ids_.end();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id)  const{
    
    auto answer = document_word_freq_.find(document_id);
    if (answer!=document_word_freq_.end()) {
        return answer ->second;
    } else {
        static map<string_view, double> empty;
        return empty;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    auto erase=documents_.erase(document_id);
    if (erase) {
        for (auto [word,freq] : document_word_freq_.at(document_id)) {
            if (word_to_document_freqs_[word].size()==1) {
                word_to_document_freqs_.erase(word);
            } else {
                word_to_document_freqs_[word].erase(document_id);
            }
        }
        document_word_freq_.erase(document_id);
        document_ids_.erase(document_id);       
    }
}

void SearchServer::RemoveDocument( std::execution::sequenced_policy seq, int document_id) {
        RemoveDocument(document_id);
}

void SearchServer::RemoveDocument( std::execution::parallel_policy par, int document_id) {
    auto erase=documents_.erase(document_id);
    if (erase) {
        vector <string> s(document_word_freq_.at(document_id).size());
        int i=0;
        for (auto [word,freq] : document_word_freq_.at(document_id)) {
            s[i]=word;
            ++i;
        }
        auto erase = [this,document_id](const auto& word ) { 
            if (word_to_document_freqs_[word].size()==1) {
                word_to_document_freqs_.erase(word);
                    
            }
            word_to_document_freqs_[word].erase(document_id);
        };  
        for_each(execution::par, s.begin(), s.end(),erase);
        document_word_freq_.erase(document_id);
        document_ids_.erase(document_id);
    }           
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument( string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    vector<string_view> matched_words;
    bool clear=false;
    for (const auto& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            clear=true;
            break;
        }
    }
    if (!clear) {
        for (const auto& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&,  std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&,  std::string_view raw_query, int document_id) const {
    const auto query = ParseQuery(raw_query);
    vector<string_view> matched_words;
    auto match= [this,&document_id,&matched_words](auto& word){
        if (word_to_document_freqs_.count(word) != 0) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
            }
        }
    };
    bool clear=false;
    auto match_clear=[this,&document_id,&matched_words,&clear](auto& word){
        if (!clear && word_to_document_freqs_.count(word) != 0) {
            if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            clear=true;
            }
        }
    };
    for_each(execution::par, query.minus_words.begin(), query.minus_words.end(),match_clear);
    if (!clear) {
        for_each(execution::par, query.plus_words.begin(), query.plus_words.end(),match);

    }
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord( string_view word) const {
    string str;
    for (auto c : word) {
        str.push_back(c);
    }
    return stop_words_.count(str) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const {
    vector<string> words;
    for (const string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}
vector<string_view> SearchServer::SplitIntoWordViewsNoStop( string_view text) const {
    vector<string_view> words;
    for (const auto& word : SplitIntoWordsView(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + word.data() + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord( string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    auto word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + text.data() + " is invalid");
    }
    return {word, is_minus, IsStopWord(word)};
}


SearchServer::Query SearchServer::ParseQuery( string_view text) const {
    Query result;
    for (const auto& word : SplitIntoWordsView(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.insert(query_word.data);
            } else {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

// Existence required
double SearchServer::ComputeWordInverseDocumentFreq( string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

    