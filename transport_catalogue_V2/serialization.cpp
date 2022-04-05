#include "serialization.h"

#include <variant>
#include <string_view>
#include <iostream>


namespace serialization {

namespace {

using namespace std::literals;

size_t GetStopId (transport_catalogue_proto::DB& db, std::string_view stop_name) {
    
    auto result = std::find_if(db.stops().begin(), db.stops().end(), 
            [stop_name] (transport_catalogue_proto::Stop stop){
                return stop.name() == stop_name;
    });
    return result->id();
}

ItProtoStop GetStopProto (google::protobuf::RepeatedPtrField<transport_catalogue_proto::Stop>* stops, uint64_t id) {
    auto result = std::find_if((*stops).begin(), (*stops).end(), 
            [id] (transport_catalogue_proto::Stop stop){
                return stop.id() == id;
    });
   
    return result;
}

struct ProtoColorSetter {
    
    transport_catalogue_proto::Color& proto_color;

    void operator()(std::monostate) const {
        //std::cout << "No roots"sv << std::endl;
    }
    void operator()(svg::Rgb color) const {
        transport_catalogue_proto::ColorRGB proto_color_rgb;
        proto_color_rgb.set_r(color.red);
        proto_color_rgb.set_g(color.green);
        proto_color_rgb.set_b(color.blue);
        *proto_color.mutable_color_rgb() = proto_color_rgb;
    }
    void operator()(svg::Rgba color) const {
        transport_catalogue_proto::ColorRGBA proto_color_rgba;
        proto_color_rgba.set_r(color.red);
        proto_color_rgba.set_g(color.green);
        proto_color_rgba.set_b(color.blue);
        proto_color_rgba.set_alpha(color.opacity);
        *proto_color.mutable_color_rgba() = proto_color_rgba;
    }
    void operator()(std::string color) const {
        transport_catalogue_proto::ColorString proto_color_string;
        proto_color_string.set_color(color);
       *proto_color.mutable_str_color() = proto_color_string;
    }
    
};

transport_catalogue_proto::Color ConvertColorSVGtoProtoColor (svg::Color svg_color) {
    transport_catalogue_proto::Color proto_color;
    std::visit(ProtoColorSetter{proto_color},svg_color);
    return proto_color;
}

svg::Color ConvertProtoColortoColorSVG (const transport_catalogue_proto::Color& proto_color) {
    svg::Color svg_color;
    if (proto_color.has_color_rgb()) {
        return svg_color = svg::Rgb(proto_color.color_rgb().r(),
                     proto_color.color_rgb().g(),
                     proto_color.color_rgb().b());
    } else if ( proto_color.has_color_rgba()) {
        return svg_color = svg::Rgba(proto_color.color_rgba().r(),
                     proto_color.color_rgba().g(),
                     proto_color.color_rgba().b(),
                     proto_color.color_rgba().alpha());
    } else if (proto_color.has_str_color()) {
        return svg_color = proto_color.str_color().color();
    } else {
        return std::monostate{};
    }
}

void StopLoad (ProtoStops proto_stops, catalogue::TransportCatalogue& tc) {
    
    for (const auto& proto_stop : proto_stops) {
        catalogue::detail::Stop stop;
        stop.name = proto_stop.name();
        stop.coordinates.lat = proto_stop.latitude();
        stop.coordinates.lng = proto_stop.longitude();
        tc.AddStop(std::move(stop));
    }
}

void StopWithLengthLoad (transport_catalogue_proto::DB db, catalogue::TransportCatalogue& tc) {
    auto proto_stops = db.stops();
    for (const auto& proto_stop : proto_stops) { 
        const std::string& stop = proto_stop.name();
        std::unordered_map<std::string,double> road_distances;
        for (const auto& proto_road_distance : proto_stop.road_distances()) {
            auto stop = GetStopProto (db.mutable_stops(), proto_road_distance.to_stop_id());
            road_distances[stop->name()] = proto_road_distance.distances();
            
        }
        tc.AddStopWithLength(std::move(stop),road_distances);
    }
}

void BusLoad (transport_catalogue_proto::DB db, catalogue::TransportCatalogue& tc ) {
    const ProtoBuses buses = db.buses();
    for (const auto& proto_bus : buses) {
        catalogue::detail::Bus bus;
        bool first = true;
        bus.name = proto_bus.name();
        for (const auto& proto_stop_id : proto_bus.stops_id()) {
            auto proto_stop = GetStopProto(db.mutable_stops(), proto_stop_id);
            auto stop = tc.FindStop(proto_stop->name());
            bus.stops.push_back(stop);
        }
        for (const auto& proto_stop_id : proto_bus.end_stops_id()) {
            auto proto_stop = GetStopProto(db.mutable_stops(), proto_stop_id);
            auto stop = tc.FindStop(proto_stop->name());
            bus.end_stops.push_back(stop);
        }
        tc.AddBus(std::move(bus)); 
    }
}

graph::GraphSettings LoadGraphSetting (transport_catalogue_proto::DB& db) {
    graph::GraphSettings graph_setting;
    graph_setting.speed = db.graph_setting().speed();
    graph_setting.wait = db.graph_setting().wait();
    return graph_setting;
}

renderer::RenderSettings LoadRenderSetting (transport_catalogue_proto::DB& db) {
    renderer::RenderSettings render_setting;
    render_setting.width = db.render_setting().width();
    render_setting.height = db.render_setting().height();
    render_setting.padding = db.render_setting().padding();
    render_setting.line_width = db.render_setting().line_width();
    render_setting.stop_radius = db.render_setting().stop_radius();
    render_setting.bus_label_font_size = db.render_setting().bus_label_font_size();
    render_setting.bus_label_offset_dx = db.render_setting().bus_label_offset_dx();
    render_setting.bus_label_offset_dy = db.render_setting().bus_label_offset_dy();
    render_setting.stop_label_font_size = db.render_setting().stop_label_font_size();
    render_setting.stop_label_offset_dx = db.render_setting().stop_label_offset_dx();
    render_setting.stop_label_offset_dy = db.render_setting().stop_label_offset_dy();
    render_setting.underlayer_color = ConvertProtoColortoColorSVG(db.render_setting().underlayer_color());
    render_setting.underlayer_width = db.render_setting().underlayer_width();
    for (size_t i = 0; i < db.render_setting().colors_pallete_size(); ++i) {
        auto color = db.render_setting().colors_pallete(i);
        render_setting.color_palette.push_back(ConvertProtoColortoColorSVG(color));
    }
    return render_setting;
}

void SaveGpraphsetting (transport_catalogue_proto::DB& db, graph::TransportRouter& trouter ) {
    transport_catalogue_proto::GraphSetting proto_graph_setting;
    auto graph_settings = trouter.GetGraphSetting();
    proto_graph_setting.set_speed(graph_settings.speed);
    proto_graph_setting.set_wait(graph_settings.wait); 
    *db.mutable_graph_setting() = proto_graph_setting;
}

void SaveRenderSettings (transport_catalogue_proto::DB& db, renderer::MapRenderer& map_renderer ) {
    transport_catalogue_proto::RenderSettings proto_render_setting;
    auto render_setting = map_renderer.GetRendererSettings();
    proto_render_setting.set_width(render_setting.width);
    proto_render_setting.set_height(render_setting.height);
    proto_render_setting.set_padding(render_setting.padding);
    proto_render_setting.set_line_width(render_setting.line_width);
    proto_render_setting.set_stop_radius(render_setting.stop_radius);
    proto_render_setting.set_bus_label_font_size(render_setting.bus_label_font_size);
    proto_render_setting.set_bus_label_offset_dx(render_setting.bus_label_offset_dx);
    proto_render_setting.set_bus_label_offset_dy(render_setting.bus_label_offset_dy);
    proto_render_setting.set_stop_label_font_size(render_setting.stop_label_font_size);
    proto_render_setting.set_stop_label_offset_dx(render_setting.stop_label_offset_dx);
    proto_render_setting.set_stop_label_offset_dy(render_setting.stop_label_offset_dy);
    *proto_render_setting.mutable_underlayer_color() = ConvertColorSVGtoProtoColor(render_setting.underlayer_color);
    proto_render_setting.set_underlayer_width(render_setting.underlayer_width);
    for (size_t i = 0; i < render_setting.color_palette.size(); ++i) {
        *proto_render_setting.add_colors_pallete() = ConvertColorSVGtoProtoColor(render_setting.color_palette[i]);
    }
    *db.mutable_render_setting() = proto_render_setting;
}

} //namespace

void SaveDataBase (Settings settings, 
                        catalogue::TransportCatalogue& tc, 
                        renderer::MapRenderer& map_renderer,
                        graph::TransportRouter& trouter) {
    std::ofstream of(settings.path, std::ios::binary);
    std::string str = settings.path;
    if (!of) {
        
    }
    transport_catalogue_proto::DB db;
    SaveRenderSettings (db, map_renderer );
    SaveGpraphsetting (db, trouter);
    
    auto all_stops = tc.GetAllStops();
    
    for (const auto stop : all_stops) {
        transport_catalogue_proto::Stop proto_stop;
        proto_stop.set_id(reinterpret_cast<uint64_t>(tc.FindStop(stop.name)));
        proto_stop.set_latitude(stop.coordinates.lat);
        proto_stop.set_longitude(stop.coordinates.lng);
        proto_stop.set_name(stop.name);

        *db.add_stops() = proto_stop;
    }
    for (const auto& [pair_stop, distance] : tc.GetStopDistanceInfo()) {
        auto stop = GetStopProto(db.mutable_stops(), reinterpret_cast<uint64_t>(pair_stop.first));
        transport_catalogue_proto::RoadDistances road_distance;
        road_distance.set_to_stop_id(reinterpret_cast<uint64_t>(pair_stop.second));
        road_distance.set_distances(distance);
        *stop->add_road_distances() = road_distance;
    }
    
    for (const auto bus : tc.GetAllBus()) {
        transport_catalogue_proto::Bus proto_bus;
        proto_bus.set_id(reinterpret_cast<uint64_t>(tc.FindBus(bus.name)));
        proto_bus.set_name(bus.name);
        for (const auto bus_stop : bus.stops) {
            proto_bus.add_stops_id(GetStopId(db,bus_stop->name)); 
        }
        for (const auto bus_stop : bus.end_stops) {
            proto_bus.add_end_stops_id(GetStopId(db,bus_stop->name)); 
        }
        *db.add_buses() = proto_bus;
    }

    
    db.SerializeToOstream(&of);
}

LoadSetting LoadBaseFromProto (Settings settings, catalogue::TransportCatalogue& tc) {
    LoadSetting load_setting;
    std::ifstream ifs (settings.path);
    transport_catalogue_proto::DB db;
    db.ParseFromIstream(&ifs);
    StopLoad(db.stops(), tc);
    StopWithLengthLoad(db, tc);
    BusLoad(db,tc);
    load_setting.graph_setting = LoadGraphSetting(db);
    load_setting.render_setting = LoadRenderSetting(db);
    return load_setting;
}

} //serialization
