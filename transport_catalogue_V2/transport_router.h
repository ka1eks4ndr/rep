#pragma once

#include "graph.h"
#include "transport_catalogue.h"
#include "router.h"

#include <map>
#include <string>
#include <optional>
#include <string_view>

namespace graph {
using namespace catalogue::detail;

struct EdgeBus {
    std::string name;
    VertexEdgeType type;
    double distance = 0.;
    int span_count = 0;

    bool operator<(const EdgeBus& other) const; 
};

struct Vertex {
    const Stop* stop;
    VertexEdgeType type;

    bool operator<(const Vertex& other) const;
};

class TransportRouter {
public:
    TransportRouter(int wait, double speed, const std::set<Bus> buses, 
                    std::unordered_set<const Stop*> stops_with_bus,
                    const catalogue::TransportCatalogue& tc);

    graph::DirectedWeightedGraph<double> GetGraph ();
            
    std::optional<Route> GetRoute(const std::string_view& from, const std::string_view& to) const;

    const graph::GraphSettings GetGraphSetting () const;

private:
    graph::VertexId GetVertexId (const Vertex& vertex);

    std::optional<graph::VertexId> GetVertexId (const Vertex& vertex) const;
   

    graph::Edge<double> CreateEdgeWait (const Stop* stop);

    graph::Edge<double> CreateEdgeDistance (const Stop* stop_from, const Stop* stop_to, double weight);

    template<typename It>
    void CreatEdgeBus (const std::string& bus_name, It begin, It end);

    void PrepareGrpah();

    const int wait_;
    const double speed_;
    const std::set<catalogue::detail::Bus> buses_;
    graph::DirectedWeightedGraph<double> grpah_;
    const catalogue::TransportCatalogue tc_;

    std::map<EdgeBus,graph::EdgeId > bus_edge_;
    std::map<graph::EdgeId,EdgeBus > edge_bus_;
    std::map<Vertex, graph::VertexId> stops_vertexid_;
    std::map<graph::VertexId, Vertex> vertexid_stops_;
};

} //namespace graph {
