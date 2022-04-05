#include "request_handler.h"



using namespace detail;


RequestHandler::RequestHandler(const TransportCatalogue& db  ,const renderer::MapRenderer& renderer, const graph::TransportRouter& trouter) 
                :db_(db),
                renderer_(renderer),
                trouter_(trouter) {
}

// Возвращает информацию о маршруте (запрос Bus)
std::optional<BusInfo> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    
    auto bus = db_.FindBus(bus_name);
    if (!(bus)) {
        return std::nullopt;
    }
    return db_.GetBusInfo(*bus);
    
    
}

// Возвращает маршруты, проходящие через
const std::set<BusPtr>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    auto stop = db_.FindStop(stop_name);
    if (!stop) {
        return nullptr;
    }
    return db_.GetStopInfo(stop_name);
}
    
svg::Document RequestHandler::RenderMap() const {
    svg::Document doc;

    const auto buses = db_.GetAllBus();
    DrawAllBuses (doc,buses);
    DrawAllBusLabel (doc,buses);
    
    auto all_stops_wiht_buses = db_.GetAllStopWithBus();
    std::set<Stop>lexicographic_stops;
    for (const auto stop : all_stops_wiht_buses) {
        lexicographic_stops.insert(*stop);
    }
    DrawAllStops(doc,lexicographic_stops);
    DrawAllStopLabel(doc,lexicographic_stops);
    
    return doc;
    
}

void RequestHandler::DrawAllBuses (svg::Document& doc, const std::set<detail::Bus>& buses) const {
    auto color_pallete_size = renderer_.GetPallete()->size();
    size_t color_index = 0;
    for (const auto& bus : buses ) {
        if (!bus.stops.empty()) {
            std::vector<geo::Coordinates>coordinates;
            for (const auto& stop :bus.stops) {
                coordinates.push_back(stop->coordinates);
            }
            doc.Add(renderer_.DrawBus(coordinates,color_index));
            color_index == color_pallete_size-1 ? color_index = 0 : ++color_index;
        }
        
    }
}

void RequestHandler::DrawAllBusLabel (svg::Document& doc, const std::set<detail::Bus>& buses) const {
    auto color_pallete_size = renderer_.GetPallete()->size();
    size_t color_index = 0;
    for (const auto& bus : buses ) {
        if (!bus.stops.empty()) {
            std::vector<geo::Coordinates>coordinates;
            for (const auto& stop :bus.end_stops) {
                doc.Add(renderer_.DrawLabelBus(stop->coordinates,bus.name));
                doc.Add(renderer_.DrawNameBus(stop->coordinates,bus.name,color_index));
            }
            color_index == color_pallete_size-1?color_index = 0 : ++color_index;
        }
    }
}

void RequestHandler::DrawAllStops (svg::Document& doc, const std::set<detail::Stop>& stops) const {
    for (const auto& stop : stops ) {
        doc.Add(renderer_.DrawStops(stop.coordinates));
    }
}

void RequestHandler::DrawAllStopLabel (svg::Document& doc, const std::set<detail::Stop>& stops) const {
    for (const auto& stop : stops ) {
        doc.Add(renderer_.DrawLabelStop(stop.coordinates,stop.name));
        doc.Add(renderer_.DrawNameStop(stop.coordinates,stop.name));
    }
}

std::optional<graph::Route> RequestHandler::GetRoute(const std::string_view& from, const std::string_view& to) {
    
    return trouter_.GetRoute(from,to);

}
    
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
