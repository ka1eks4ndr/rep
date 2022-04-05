#include "transport_catalogue.h"
#include "geo.h"

#include <cassert>
#include <unordered_set>

    
namespace catalogue {



TransportCatalogue::TransportCatalogue()=default;

TransportCatalogue::~TransportCatalogue()=default;

void TransportCatalogue::AddBus (detail::Bus bus) {
    buses_.push_back(std::move(bus));
    const auto last_added_bus=&buses_.back();
    n_buses_[last_added_bus->name]=last_added_bus;
    for (auto stop : last_added_bus->stops) {
        stops_buses_[stop].insert(last_added_bus);
    }

}

void TransportCatalogue::AddStop (detail::Stop Stop){
    stops_.push_back(std::move(Stop));
    auto last_added_stop=&stops_.back();
    n_stops_[last_added_stop->name]=last_added_stop;
    stops_buses_[last_added_stop];
}

void TransportCatalogue::AddStopWithLength (std::string stop, std::unordered_map<std::string,double> lenght_to_stops) {
    if (!lenght_to_stops.empty()) {
        auto stop1=FindStop(stop);
        for (auto& [stop,length] : lenght_to_stops) {
            auto stop2=FindStop(stop);
            std::pair<const detail::Stop*,const detail::Stop*> elemet={stop1,stop2};
            route_length_[elemet]=length;
        }
    }
}

const detail::Stop* TransportCatalogue::FindStop (const std::string_view& name) const {
    if (!(n_stops_.count(name))) {
        return nullptr;
    } 
return n_stops_.at(name);
}

const detail::Bus* TransportCatalogue::FindBus (const std::string_view& name) const {
    if (!(n_buses_.count(name))) {
        return nullptr;
    }
    return n_buses_.at(name);
}

double TransportCatalogue::GetLenght (const std::string_view& stop_from, const std::string_view& stop_to) const {
    auto ptr_stop_from =FindStop(stop_from);
    auto ptr_stop_to= FindStop(stop_to);

    if (route_length_.count({ptr_stop_from,ptr_stop_to})) {
        return route_length_.at({ptr_stop_from,ptr_stop_to});
    }
    return route_length_.at({ptr_stop_to,ptr_stop_from});
}

detail::BusInfo TransportCatalogue::GetBusInfo(const detail::Bus& bus) const {
    std::unordered_set <const detail::Stop*> filter;
    detail::BusInfo businfo;
    
    auto first_stop=bus.stops.begin();
    auto next_stop=first_stop+1;
    filter.insert(*first_stop);
    double direct_lenght=0;
    
    
    while (next_stop!=bus.stops.end()) {
        filter.insert(*next_stop);
        direct_lenght+=ComputeDistance((*first_stop)->coordinates,(*next_stop)->coordinates);
        businfo.route_length+=GetLenght((*first_stop)->name,(*next_stop)->name);
        first_stop=next_stop;
        ++next_stop;
    }
    businfo.curvature=businfo.route_length/direct_lenght;
    businfo.stop_numbers=bus.stops.size();
    businfo.uniqe_stop_numbers=filter.size();
    return businfo;
}

const std::set<const catalogue::detail::Bus*>* TransportCatalogue::GetStopInfo(const std::string_view& stop) const {
    auto ptr_stop = FindStop(stop);
    assert(ptr_stop);
    
    return &(stops_buses_.at(ptr_stop));
}

const std::unordered_set<const detail::Stop*> TransportCatalogue::GetAllStopWithBus() const{
    std::unordered_set<const detail::Stop*> result;
    for (const auto& [stop, buses] : stops_buses_) {
        if (!buses.empty()) {
            result.insert(stop);
        }
    }
    return result;
}

const std::set<detail::Bus> TransportCatalogue::GetAllBus() const {
    return std::set<detail::Bus>(buses_.begin(), buses_.end());
}

const std::set<detail::Stop> TransportCatalogue::GetAllStops() const {
    return std::set<detail::Stop>(stops_.begin(), stops_.end());
}

const std::unordered_map<std::pair<const detail::Stop*,const detail::Stop*>,double, detail::PairStopsHasher> TransportCatalogue::GetStopDistanceInfo() const {
    return route_length_;
}

} //namespace catalogue