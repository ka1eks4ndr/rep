#include "json_reader.h"
#include "json_builder.h"

#include <cassert>
#include <sstream>    

using namespace std::literals;

std::stringstream ConvertStreamSVGtoJSON (std::istream& stream) {
    std::stringstream output;
            output<<'\"';
            char c;
            while (stream.get(c)) {
                switch (c)
                {
                case '"' :
                    output<<'\\';
                    output<<c;
                    break;
                case '\n' :
                    output<<'\\';
                    output<<'n';
                    break;
                
                default:
                    output<<c;
                    break;
                }
                
            }
            output<<'\"';
    return output;
}

svg::Color SetColor (json::Node node) {
    svg::Color color;
    if (node.IsArray()) {
        if (node.AsArray().size()==3) {
            color = svg::Rgb(node.AsArray()[0].AsInt(),
                                        node.AsArray()[1].AsInt(),
                                        node.AsArray()[2].AsInt());
        } else {
            color = svg::Rgba(node.AsArray()[0].AsInt(),
                                        node.AsArray()[1].AsInt(),
                                        node.AsArray()[2].AsInt(),
                                        node.AsArray()[3].AsDouble());
        } 
    } else {
        color = node.AsString();
    }
    return color;
}

JsonReader::JsonReader(TransportCatalogue& tc, std::istream& stream) 
            :   document_ (json::Load(stream)),
                tc_(tc) {
    
}

    
void JsonReader::CatalogueLoader () {
    auto& base_request_arr = document_.GetRoot().AsDict().at("base_requests").AsArray();
    for (auto& dict : base_request_arr) {
        if (dict.AsDict().at("type").AsString() == "Stop") {
            StopLoad(dict.AsDict());
        }
    }
    for (auto& dict : base_request_arr) {
        if (dict.AsDict().at("type").AsString() == "Stop") {
            StopWithLengthLoad( dict.AsDict());
        }
    }
    for (auto& dict : base_request_arr) {
        if (dict.AsDict().at("type").AsString() == "Bus") {
            BusLoad( dict.AsDict());
        }
    }
}

void JsonReader::BusLoad ( const json::Dict& bus_query) {
    detail::Bus bus;
    bool first = true;
    bus.name = bus_query.at("name").AsString();
    for (const auto& stop : bus_query.at("stops").AsArray()) {
        auto ptr_stop = tc_.FindStop(stop.AsString());
        if (first) {
            bus.end_stops.push_back(ptr_stop);
            first = false;
        }
        assert(ptr_stop);
        bus.stops.push_back(ptr_stop);
    }
    
    if (!(bus_query.at("is_roundtrip").AsBool())) {
        first = true;
        for (auto it = bus_query.at("stops").AsArray().rbegin();
                                it<bus_query.at("stops").AsArray().rend();++it) {
            auto ptr_stop = tc_.FindStop((*it).AsString());
            if (first ) {
                if (bus.end_stops.back() != ptr_stop) {
                    bus.end_stops.push_back(ptr_stop);
                }
                first = false;
            } else {
                bus.stops.push_back(ptr_stop);
            }
        }
    }
    tc_.AddBus(std::move(bus));
}

void JsonReader::StopLoad ( const json::Dict& stop_query ) {
    detail::Stop stop;
    stop.name = stop_query.at("name").AsString();
    stop.coordinates.lat = stop_query.at("latitude").AsDouble();
    stop.coordinates.lng = stop_query.at("longitude").AsDouble();
    tc_.AddStop(std::move(stop));
}

void JsonReader::StopWithLengthLoad( const json::Dict& stop_query) {
    const std::string& stop = stop_query.at("name").AsString();
    std::unordered_map<std::string,double> road_distances;
    for (const auto& [stop, distance] : stop_query.at("road_distances").AsDict()) {
        road_distances[stop] = distance.AsDouble();
    }
    tc_.AddStopWithLength(std::move(stop),road_distances);
}

void JsonReader::ProcessingStatRequests ( RequestHandler& rh) {
    auto& base_stat_requests = document_.GetRoot().AsDict().at("stat_requests").AsArray();
    json::Array result;
    for (auto& dict : base_stat_requests ) {
        int request_id = dict.AsDict().at("id").AsInt();
        if (dict.AsDict().at("type").AsString() == "Map") {
            result.emplace_back(GetMap(rh,request_id));
        }
        if (dict.AsDict().at("type").AsString() == "Bus") {
            result.emplace_back(BusInfo(rh,request_id,dict));
        } else if (dict.AsDict().at("type").AsString() == "Stop") {
            result.emplace_back(StopInfo(request_id,dict));
        } else if (dict.AsDict().at("type").AsString() == "Route") {
            result.emplace_back(GetRoute(rh,request_id,dict));
        }
    }
    json::Print(json::Document{result},std::cout);
}

json::Node JsonReader::GetMap(RequestHandler& rh, int request_id) {
    std::stringstream stream;
    rh.RenderMap().Render(stream);
    std::stringstream json_stream = ConvertStreamSVGtoJSON(stream);
            
    return json::Builder{}
                .StartDict()
                    .Key("map").Value((json::Load(json_stream)).GetRoot().AsString())
                    .Key("request_id").Value(request_id)
                .EndDict()
            .Build();
}

json::Node JsonReader::BusInfo (RequestHandler& rh, int request_id,const json::Node& dict) {
    auto bus_info = rh.GetBusStat(dict.AsDict().at("name").AsString());
    if (bus_info) {
        return
            json::Builder{}
                .StartDict()
                    .Key("curvature").Value(bus_info->curvature)
                    .Key("request_id").Value(request_id)
                    .Key("route_length").Value(bus_info->route_length)
                    .Key("stop_count").Value(bus_info->stop_numbers)
                    .Key("unique_stop_count").Value(bus_info->uniqe_stop_numbers)
                .EndDict()
            .Build();
    } else {
        return
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("error_message").Value("not found")
                .EndDict()
            .Build();
    }
}
json::Node JsonReader::StopInfo (int request_id,const json::Node& dict) {
    auto find_stop = tc_.FindStop(dict.AsDict().at("name").AsString());
    if (!find_stop) {
        return 
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("error_message").Value("not found")
                .EndDict()
            .Build();
    } else {
        auto stop_buses = tc_.GetStopInfo(find_stop->name);
        json::Array buses;
        std::set<std::string>lexicographic_buses;
        for (const auto& bus : *stop_buses) {
            lexicographic_buses.insert(bus->name);
        }
        for (const auto& bus : lexicographic_buses) {
            buses.emplace_back(bus);
        }
        return
            json::Builder{}
                .StartDict()
                    .Key("buses").Value(buses)
                    .Key("request_id").Value(request_id)
                .EndDict()
            .Build();
    }
}

json::Node JsonReader::GetRoute (RequestHandler& rh, int request_id,const json::Node& dict) {
    auto& stop_from = dict.AsDict().at("from").AsString();
    auto& stop_to = dict.AsDict().at("to").AsString();
    
    auto route_info = rh.GetRoute(stop_from,stop_to);
    if (!route_info) {
        return 
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("error_message").Value("not found")
                .EndDict()
            .Build();
    } else {
        json::Array buses;
        for (const auto& part_route : (*route_info).part_route) {
            if (part_route.type == graph::VertexEdgeType::DISTANCE) {
                json::Node node = json::Builder{}
                                        .StartDict()
                                            .Key("type").Value("Bus")
                                            .Key("bus").Value(part_route.name)
                                            .Key("span_count").Value(part_route.span_count)
                                            .Key("time").Value(part_route.time)
                                        .EndDict()
                                    .Build();
                buses.emplace_back(node);    

            } else {
                json::Node node = json::Builder{}
                                        .StartDict()
                                            .Key("type").Value("Wait")
                                            .Key("stop_name").Value(part_route.name)
                                            .Key("time").Value(part_route.time)
                                        .EndDict()
                                    .Build();
                buses.emplace_back(node);
            }
        }
        return 
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(request_id)
                    .Key("total_time").Value((*route_info).waight)
                    .Key("items").Value(buses)
                .EndDict()
            .Build();

    }
}

renderer::RenderSettings JsonReader::SetRenderSettings () {
    renderer::RenderSettings rs;
    auto& render_settings = document_.GetRoot().AsDict().at("render_settings").AsDict();
    
    rs.width = render_settings.at("width").AsDouble();
    rs.height = render_settings.at("height").AsDouble();
    rs.padding = render_settings.at("padding").AsDouble();
    rs.line_width = render_settings.at("line_width").AsDouble();
    rs.stop_radius = render_settings.at("stop_radius").AsDouble();
    rs.bus_label_font_size = render_settings.at("bus_label_font_size").AsInt();
    rs.bus_label_offset_dx = render_settings.at("bus_label_offset").AsArray()[0].AsDouble();
    rs.bus_label_offset_dy = render_settings.at("bus_label_offset").AsArray()[1].AsDouble();
    rs.stop_label_font_size = render_settings.at("stop_label_font_size").AsInt();
    rs.stop_label_offset_dx = render_settings.at("stop_label_offset").AsArray()[0].AsDouble();
    rs.stop_label_offset_dy = render_settings.at("stop_label_offset").AsArray()[1].AsDouble();
    auto underlayer_color = render_settings.at("underlayer_color");
    rs.underlayer_color = SetColor(underlayer_color);
    rs.underlayer_width = render_settings.at("underlayer_width").AsDouble();
    auto color_palette  = render_settings.at("color_palette").AsArray();
    for (size_t i = 0; i<color_palette.size();++i) {
        auto color = color_palette[i];
        rs.color_palette.push_back(SetColor(color));
    }
    return rs;
    
}

serialization::Settings JsonReader::GetSerialSettings () {
    serialization::Settings settings;
    auto& jr_settings = document_.GetRoot().AsDict().at("serialization_settings").AsDict();
    settings.path = jr_settings.at("file").AsString();
    return settings;
}

graph::GraphSettings JsonReader::SetGraphSettings () {
    graph::GraphSettings gs;
    auto& graph_settings = document_.GetRoot().AsDict().at("routing_settings").AsDict();
    gs.wait = graph_settings.at("bus_wait_time").AsDouble();
    gs.speed = graph_settings.at("bus_velocity").AsInt();
    return gs;
}


  