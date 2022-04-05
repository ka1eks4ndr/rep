#pragma once

#include "concurrent_map.h"
#include "document.h"
#include "log_duration.h"
#include "string_processing.h"

#include <algorithm>
#include <execution>
#include <map>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

using namespace std::string_literals;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
    {
        if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
            throw std::invalid_argument("Some of stop words are invalid");
        }
    }

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id,  std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments( std::string_view raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);
         std::vector<Document> matched_documents;
        matched_documents = FindAllDocuments(query, document_predicate);
        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments( std::execution::sequenced_policy seq, std::string_view raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments( std::execution::parallel_policy par, std::string_view raw_query, DocumentPredicate document_predicate) const {
        const auto query = ParseQuery(raw_query);

        auto matched_documents = FindAllDocuments(par, query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }
    
    std::vector<Document> FindTopDocuments( std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments( std::execution::sequenced_policy seq, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments( std::execution::parallel_policy par, std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments( std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments( std::execution::sequenced_policy seq, std::string_view raw_query) const;

    std::vector<Document> FindTopDocuments( std::execution::parallel_policy par, std::string_view raw_query) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index) const;

    std::set<int>::iterator begin();
    
    std::set<int>::iterator end ();

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    void RemoveDocument(int document_id);

    void RemoveDocument( std::execution::sequenced_policy seq, int document_id);

    void RemoveDocument( std::execution::parallel_policy par, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument( std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy&,  std::string_view raw_query, int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy&,  std::string_view raw_query, int document_id) const;
    
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };

    const std::set<std::string> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    //std::vector<int> document_ids_;
    std::set<int> document_ids_;
    std::map<int,std::map<std::string_view,double>> document_word_freq_;
    std::map<int,std::string>documents_strings_;

    bool IsStopWord( std::string_view word) const;
    static bool IsValidWord( std::string_view word);
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;
    std::vector<std::string_view> SplitIntoWordViewsNoStop( std::string_view text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
    QueryWord ParseQueryWord(std::string_view text) const;
    Query ParseQuery( std::string_view text) const;
    double ComputeWordInverseDocumentFreq( std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy par, const Query& query, DocumentPredicate document_predicate) const {
        ConcurrentMap<int,double> document_to_relevance(16);
        for (const auto& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for_each(std::execution::par,word_to_document_freqs_.at(word).begin(),word_to_document_freqs_.at(word).end(),[this,&document_to_relevance,
                                                                                                        inverse_document_freq,document_predicate]
                                                                                                        (std::pair<int,double> id_term){
                    const auto& document_data = this->documents_.at(id_term.first);
                    if (document_predicate(id_term.first, document_data.status, document_data.rating)) {
                        document_to_relevance[id_term.first].ref_to_value += id_term.second * inverse_document_freq;
                    }
                }
            );

        }
        
        for (const auto& word : query.minus_words) {
            
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for_each(std::execution::par,word_to_document_freqs_.at(word).begin(),word_to_document_freqs_.at(word).end(),[&document_to_relevance]
                                                                                                                        (std::pair<int,double> id_term){
                                                                                                                            document_to_relevance.erase(id_term.first);
                                                                                                                        });
        }
        auto doc_to_rel=document_to_relevance.BuildOrdinaryMap();
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : doc_to_rel) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
    
    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (const auto& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }    
        for (const auto& word : query.minus_words) {
            
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};