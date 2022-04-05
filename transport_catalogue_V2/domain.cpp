#include "domain.h"

namespace catalogue {

namespace detail {

size_t PairStopsHasher::operator()(const std::pair<const detail::Stop*,const detail::Stop*>pairstop) const {
    size_t hash_stop1= point_hasher_(pairstop.first);
    size_t hash_stop2= point_hasher_(pairstop.second);
    return hash_stop1+hash_stop2*37;
}

//-----------------------Stop-----------------------------------
    bool Stop::operator<(const Stop& other) const {
        return name<other.name;
    }

//-----------------------Bus------------------------------------  
    bool Bus::operator<(const Bus& other) const {
        return name<other.name;
    }
   
} //namespace catalogue 

} //namespace detail 