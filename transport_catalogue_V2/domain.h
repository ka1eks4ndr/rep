#pragma once

#include "geo.h"
#include "svg.h"

#include <string>
#include <vector>
#include <set>
#include <map>
#include <utility>

namespace catalogue {
namespace detail {

struct Stop  
{   
    bool operator<(const Stop& other) const;
    std::string name;
    geo::Coordinates coordinates;
};

class PairStopsHasher {
public:
    size_t operator()(const std::pair<const detail::Stop*,const detail::Stop*>pairstop) const;
private:
    std::hash<const void*> point_hasher_;
};

struct Bus
{   
    bool operator<(const Bus& other) const;
    std::string name;
    std::vector<const Stop*>stops;
    std::vector<const Stop*>end_stops;

};

struct BusInfo
{
    int stop_numbers = 0;
    int uniqe_stop_numbers = 0;
    double route_length = 0;
    double curvature = 0;
};



} //namespace catalogue 

} //namespace detail 

namespace svg {

} //namespace svg 

namespace graph {

struct GraphSettings 
{
    int wait; 
    double speed;
};

enum class VertexEdgeType {
    WAIT,
    DISTANCE
};

struct PartRoute {
    VertexEdgeType type;
    const std::string name;
    const double time = 0;
    const int span_count = 0;

};

struct Route {
    double waight = 0.;
    std::vector<PartRoute> part_route;
};

} //namespace graph 

namespace renderer {

struct RenderSettings {
    double width = 0;
    double height = 0;
    double padding = 0;
    double line_width = 0;
    double stop_radius = 0;
    int bus_label_font_size = 0;
    double bus_label_offset_dx = 0;
    double bus_label_offset_dy = 0;
    int stop_label_font_size = 0;
    double stop_label_offset_dx = 0;
    double stop_label_offset_dy = 0;
    svg::Color  underlayer_color;
    double underlayer_width = 0;
    std::vector<svg::Color>color_palette;
};

} //namespace renderer

namespace serialization {
struct LoadSetting {
    renderer::RenderSettings render_setting;
    graph::GraphSettings graph_setting;
};
} //namespace serialization