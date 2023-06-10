#pragma once

#include <transport_catalogue.pb.h>

#include "json_builder.h"
#include "json.h"
#include "deserialization.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "router.h"
#include "graph.h"

#include <vector>
#include <iostream>
#include <optional>
#include <unordered_map>

class Seriallization{
public:
    Seriallization(std::istream& input = std::cin);

private:
    json::Document doc;
    transport::Catalogue catalog;
    render_settings::Settings settings;
    save::Graph graph;
    save::RoutesInternalData routes_internal_data;
    save::TransportRoute transport_route;

    std::vector <const json::Node*> queue_stops;
    std::vector <const json::Node*> queue_buses;
    std::vector <std::pair <std::string_view, const json::Node*>> queue_lengths;

    TransportCatalogue saved_catalog;
    std::optional<TransportRouter> saved_router;

    void CreateQueue();

    void ParseQueueStops();

    void ParseQueueBuses();

    void ParseQueueLengths();

    void ParseRenderSettings();

    render_settings::Color ParseColor(const json::Node& node) const;

    void BuildRouter();

    void StartParseRouter();

    void ParseSaveGraph();

    void ParseSaveRouter();

    void ParseSaveTransportRoute();

    void Save();
};