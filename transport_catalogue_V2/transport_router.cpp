#include "transport_router.h"

namespace graph {
using namespace catalogue::detail;


bool EdgeBus::operator<(const EdgeBus& other) const {
    return name < other.name;
}


bool Vertex::operator<(const Vertex& other) const {
    return (stop->name< other.stop->name ) ||
            (stop->name == other.stop->name &&  type < other.type) ;
}


TransportRouter::TransportRouter(int wait, double speed, const std::set<Bus> buses, 
                std::unordered_set<const Stop*> stops_with_bus,
                const catalogue::TransportCatalogue& tc) :
            wait_(wait),
            speed_(speed),
            buses_(buses),
            grpah_(stops_with_bus.size()*2),
            tc_(tc) {
        PrepareGrpah();
}

graph::DirectedWeightedGraph<double> TransportRouter::GetGraph () {
    return grpah_;
}   
            
std::optional<Route> TransportRouter::GetRoute(const std::string_view& from, const std::string_view& to) const  {
    auto stop_from = GetVertexId({tc_.FindStop(from),VertexEdgeType::WAIT});
    auto stop_to = GetVertexId({tc_.FindStop(to),VertexEdgeType::WAIT});
    if (!stop_from || !stop_to) {
        return {};
    }
    static graph::Router<double> router(grpah_);
    auto route = router.BuildRoute(*stop_from,*stop_to);
    if (!route) {
        return {};
    } else {
        std::vector<PartRoute> bus_route;
        for (const auto& edge : (*route).edges ) {
            const auto& edge_bus = edge_bus_.at(edge);
            if (edge_bus.type == VertexEdgeType::WAIT) {
                bus_route.push_back(PartRoute{edge_bus.type, edge_bus.name,static_cast<double>(wait_),0});
            } else {
                bus_route.push_back(PartRoute{edge_bus.type, 
                                    edge_bus.name,
                                    edge_bus.distance,
                                    edge_bus.span_count
                });
            }
        }
        
        return Route{(*route).weight , bus_route};
    }
    
}

graph::VertexId TransportRouter::GetVertexId (const Vertex& vertex) {
    static graph::VertexId id = 0;
    
    if (stops_vertexid_.count(vertex) ) {
        return stops_vertexid_.at(vertex);
    }
    vertexid_stops_[id]=vertex;
    stops_vertexid_[vertex]=id;
    return id++;
}

std::optional<graph::VertexId> TransportRouter::GetVertexId (const Vertex& vertex) const {
    if (stops_vertexid_.count(vertex)) {
        return stops_vertexid_.at(vertex);
    }
    return {};
}

graph::Edge<double> TransportRouter::CreateEdgeWait (const Stop* stop) {
    graph::VertexId from_stop_wait = GetVertexId({stop,VertexEdgeType::WAIT});
    graph::VertexId to_stop_distance = GetVertexId({stop,VertexEdgeType::DISTANCE});
    graph::Edge<double> edge_wait = graph::Edge<double>{
                from_stop_wait,
                to_stop_distance,
                static_cast<double>(wait_)
        };
    return edge_wait;
}

graph::Edge<double> TransportRouter::CreateEdgeDistance (const Stop* stop_from, const Stop* stop_to, double weight) {
    VertexId from_stop_distance = GetVertexId({stop_from,VertexEdgeType::DISTANCE});
    VertexId to_stop_wait_distance = GetVertexId({stop_to,VertexEdgeType::WAIT});
    
    graph::Edge<double> edge_wait = graph::Edge<double>{
                from_stop_distance,
                to_stop_wait_distance,
                weight
        };
    return edge_wait;
}

template<typename It>
void TransportRouter::CreatEdgeBus (const std::string& bus_name, It begin, It end) {
    while (begin!=end) {
        double weight = 0;
        auto stop1 = *begin;
        auto edge_wait = CreateEdgeWait(stop1);
        edge_bus_[grpah_.AddEdge(edge_wait)]={stop1->name,VertexEdgeType::WAIT};
        auto ItNextStop = next(begin,1);
        while (ItNextStop != end) {
            auto stop2 = *ItNextStop;
            auto prev_stop =  *(next(ItNextStop,-1));
            weight+=(tc_.GetLenght(prev_stop->name,stop2->name))/1000/speed_*60;
            auto edge_distance = CreateEdgeDistance(stop1,stop2,weight);
            edge_bus_[grpah_.AddEdge(edge_distance)]={bus_name,VertexEdgeType::DISTANCE,weight,static_cast<int>(distance(begin,ItNextStop))};
            ++ItNextStop;
        }
        ++begin;
        
    }
}

void TransportRouter::PrepareGrpah() {
    //LOG_DURATION("prepare graph");
    for (const auto& bus : buses_ ) {
        if (bus.end_stops.size() != 2) {
            CreatEdgeBus(bus.name, bus.stops.begin(),bus.stops.end());
        } else {
            auto it_midle = next(bus.stops.begin(),bus.stops.size()/2+1);
            CreatEdgeBus(bus.name, bus.stops.begin(),it_midle);
            CreatEdgeBus(bus.name, --it_midle,bus.stops.end());

        }
    }
}

const graph::GraphSettings TransportRouter::GetGraphSetting () const {
    return graph::GraphSettings{wait_,speed_};
} 

} //namespace graph 
