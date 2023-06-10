#pragma once

#include <iostream>
#include <string_view>

#include "transport_catalogue.h"
#include "transport_router.h"

#include <transport_catalogue.pb.h>

class Deserialization{
public:
    Deserialization() = default;
    Deserialization(const std::string& path_file);
    TransportCatalogue GetTransportCatalogue();
    TransportCatalogue GetTransportCatalogue(const transport::Catalogue& catalog);
    const render_settings::Settings& GetRenderSettings() const;
    TransportRouter GetTransportRoute(const TransportCatalogue& catalog);
private:
    ObjectProto obj_;
    TransportCatalogue new_catalog;
    SaveTransportRouter save;

    void AddStops();
    void AddBus();
    void AddLenghts();

    void AddGraphData();
    void AddRouteData();
    void AddTransportRouteData();
};

