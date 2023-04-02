#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "transport_catalogue.h"

class InputReader{
    public:
    InputReader(TransportCatalogue& map);


    private:
    TransportCatalogue& map_;
    std::vector<std::string> queue_stops;
    std::vector<std::string> queue_buses;
    std::unordered_map <std::string, std::string> queue_lenghts;

    void CreateQueue();

    void ParseQueueStops();

    void ParseQueueBuses();

    void ParseQueueLenghts();
};