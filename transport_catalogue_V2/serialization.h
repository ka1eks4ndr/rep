#pragma once

#include "domain.h"
#include "transport_catalogue.h"
#include "transport_catalogue.pb.h"
#include "transport_router.h"
#include "map_renderer.h"

#include <filesystem>
#include <fstream>
#include <algorithm>
#include <string_view>


namespace serialization {

using Path = std::filesystem::path;
using ProtoStops = google::protobuf::RepeatedPtrField<transport_catalogue_proto::Stop>;
using ProtoBuses = google::protobuf::RepeatedPtrField<transport_catalogue_proto::Bus>;
using ItProtoStop = google::protobuf::internal::RepeatedPtrIterator<transport_catalogue_proto::Stop>;

struct Settings
{
    Path path;
};

void SaveDataBase (Settings settings, 
                        catalogue::TransportCatalogue& tc, 
                        renderer::MapRenderer& map_renderer,
                        graph::TransportRouter& trouter); 

LoadSetting LoadBaseFromProto (Settings settings, catalogue::TransportCatalogue& tc);

} //serialization
