#include "json_reader.h"
#include "map_renderer.h"
#include "transport_catalogue.h"

#include <fstream>
#include <iostream>
#include <string_view>
#include <filesystem>

using namespace std::literals;
using namespace serialization;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        catalogue::TransportCatalogue tc;
        JsonReader jr(tc, std::cin);
        jr.CatalogueLoader();
        renderer::RenderSettings render_setting = jr.SetRenderSettings();
        auto stops=tc.GetAllStopWithBus();
        std::unordered_set<geo::Coordinates,geo::CoordinatesHasher>coordinates;
        for (auto stop : stops) {
            coordinates.insert(stop->coordinates);
        }
        renderer::SphereProjector sp(coordinates.begin(),coordinates.end(),
                            render_setting.width,
                            render_setting.height,
                            render_setting.padding);
        renderer::MapRenderer map_renderer(render_setting,sp);
        graph::GraphSettings setting = jr.SetGraphSettings();
        const auto buses = tc.GetAllBus();
        graph::TransportRouter trouter(setting.wait, setting.speed,buses, stops,tc);

        SaveDataBase(jr.GetSerialSettings(),tc,map_renderer,trouter);
        

    } else if (mode == "process_requests"sv) {
        catalogue::TransportCatalogue tc;
        JsonReader jr(tc, std::cin);
        LoadSetting setting = LoadBaseFromProto(jr.GetSerialSettings(),tc);

        renderer::RenderSettings render_setting=setting.render_setting;
        auto stops=tc.GetAllStopWithBus();
         std::unordered_set<geo::Coordinates,geo::CoordinatesHasher>coordinates;
         for (auto stop : stops) {
             coordinates.insert(stop->coordinates);
         }
         renderer::SphereProjector sp(coordinates.begin(),coordinates.end(),
                             render_setting.width,
                             render_setting.height,
                             render_setting.padding);
         renderer::MapRenderer map_renderer(render_setting,sp);

        graph::GraphSettings graph_setting = setting.graph_setting;
        const auto buses = tc.GetAllBus();

        graph::TransportRouter trouter(graph_setting.wait, graph_setting.speed,buses, stops,tc);

        RequestHandler rh(tc,map_renderer,trouter);
        jr.ProcessingStatRequests(rh);

    } else {
        PrintUsage();
        return 1;
    }
}