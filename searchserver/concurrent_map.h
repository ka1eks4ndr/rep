
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <mutex>
#include <execution>


using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access {
        
        std::lock_guard<std::mutex> lock;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count):bucket_count_(bucket_count),v_map_(bucket_count_) {
        
    }

    Access operator[](const Key& key) {
        
        auto number_dict=key%bucket_count_;
            return Access{ std::lock_guard<std::mutex> (v_map_[number_dict].first),v_map_[number_dict].second[key] };

        
        
    }
    
   //5ms this section lag. and i don't know what need to do T_T 
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result_m;
        for_each(v_map_.begin(),v_map_.end(),[&result_m]
                                    (std::pair<std::mutex,std::map<Key,Value>>&pair_mut_map){
                                       result_m.insert(pair_mut_map.second.begin(),pair_mut_map.second.end());

        });
        
        return result_m;
    }

    std::size_t erase(Key key) {
            auto number_dict=key%bucket_count_;
            std::lock_guard<std::mutex>lock (v_map_[number_dict].first);
            
        
        return v_map_[number_dict].second.erase(key);
    }

private:
    const size_t bucket_count_;
    std::vector<std::pair<std::mutex,std::map<Key,Value>>> v_map_;
};
