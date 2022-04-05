#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "serialization.h"

#include <istream>

using namespace catalogue;

class JsonReader
{
public:
    JsonReader(TransportCatalogue& tc,  std::istream& stream);
    
    void CatalogueLoader ();
    void ProcessingStatRequests ( RequestHandler& rh);
    renderer::RenderSettings SetRenderSettings ();
    graph::GraphSettings SetGraphSettings ();
    
    serialization::Settings GetSerialSettings ();


private:
    void BusLoad ( const json::Dict& bus_query);
    void StopLoad ( const json::Dict& stop_query);
    void StopWithLengthLoad( const json::Dict& stop_query);

    json::Node GetMap(RequestHandler& rh, int request_id );
    json::Node GetRoute (RequestHandler& rh, int request_id, const json::Node& dict);


    json::Node BusInfo (RequestHandler& rh, int request_id,const json::Node& dict) ;
    json::Node StopInfo (int request_id,const json::Node& dict);
    json::Document document_;
    TransportCatalogue& tc_;
    
};




