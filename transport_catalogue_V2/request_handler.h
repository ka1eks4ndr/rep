#pragma once


#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "graph.h"
#include "domain.h"


#include <string>

using namespace catalogue;


using BusPtr = const detail::Bus*;

struct BusStat
{
    double curvature;
    int route_length;
    int stop_count;
    int unique_stop_count;
};

class RequestHandler {
public:
    
RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer, const graph::TransportRouter& trouter);
    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<detail::BusInfo> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;

    svg::Document RenderMap() const;

    std::optional<graph::Route>GetRoute(const std::string_view& from, const std::string_view& to) ;



private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    
    void DrawAllBuses (svg::Document& doc, const std::set<detail::Bus>& buses) const;
    void DrawAllBusLabel (svg::Document& doc, const std::set<detail::Bus>& buses) const;

    void DrawAllStops (svg::Document& doc, const std::set<detail::Stop>& stops) const;
    void DrawAllStopLabel (svg::Document& doc, const std::set<detail::Stop>& stops) const;


    const TransportCatalogue& db_;
    const renderer::MapRenderer& renderer_;
    const graph::TransportRouter& trouter_;
};
