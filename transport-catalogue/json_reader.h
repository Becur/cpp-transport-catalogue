#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_builder.h"

#include <iostream>
#include <string_view>
#include <vector>

class JsonReader{
    public:
    JsonReader(TransportCatalogue& map, std::istream& input = std::cin, std::ostream& output = std::cout);

    private:
    TransportCatalogue& map_;
    const json::Document doc;
    std::ostream& output_;
    std::vector <const json::Node*> queue_stops;
    std::vector <const json::Node*> queue_buses;
    std::vector <std::pair <std::string_view, const json::Node*>> queue_lengths;
    json::Builder array = json::Builder{};

    void CreateQueue();

    void ParseQueueStops();

    void ParseQueueBuses();

    void ParseQueueLengths();

    void PrintStat();

    json::Builder& PrintBus(const json::Node* node);

    json::Builder& PrintStop(const json::Node* node);

    json::Builder& PrintMap(const json::Node* node);
};