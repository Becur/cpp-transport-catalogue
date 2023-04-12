#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"

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

    void CreateQueue();

    void ParseQueueStops();

    void ParseQueueBuses();

    void ParseQueueLengths();

    void PrintStat();

    void PrintBus(const json::Node* node);

    void PrintStop(const json::Node* node);

    void PrintMap(const json::Node* node);
};