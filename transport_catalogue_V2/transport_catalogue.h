#pragma once

#include "domain.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <string_view>
#include <deque>

namespace catalogue {

class TransportCatalogue
{
private:
    
    std::deque<detail::Bus>buses_;
    std::deque<detail::Stop>stops_;
    std::unordered_map<std::string_view,const detail::Stop*>n_stops_;
    std::unordered_map<std::string_view,const detail::Bus*>n_buses_;
    std::unordered_map<std::pair<const detail::Stop*,const detail::Stop*>,double, detail::PairStopsHasher>route_length_;
    std::unordered_map<const detail::Stop*, std::set<const detail::Bus*>> stops_buses_;

public:
    TransportCatalogue();
    
    ~TransportCatalogue();
    
    void AddBus (detail::Bus bus);

    void AddStop (detail::Stop stop);

    void AddStopWithLength (std::string stop, std::unordered_map<std::string,double> lenght_to_stops);

    const detail::Stop* FindStop (const std::string_view& name) const;

    const detail::Bus* FindBus (const std::string_view& name) const;
    
    detail::BusInfo GetBusInfo(const detail::Bus& bus) const;

    const std::set<const detail::Bus*>* GetStopInfo(const std::string_view& stop) const;

    const std::unordered_set<const detail::Stop*> GetAllStopWithBus() const;

    const std::set<detail::Stop> GetAllStops() const;

    const std::set<detail::Bus> GetAllBus() const;

    double GetLenght (const std::string_view& stop_from, const std::string_view& stop_to) const;

    const std::unordered_map<std::pair<const detail::Stop*,const detail::Stop*>,double, detail::PairStopsHasher> GetStopDistanceInfo () const; 
};

} //namespace catalogue


