#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>

#include "transport_catalogue.h"

class InputReader{
    public:
    InputReader(TransportCatalogue& map, std::istream& input = std::cin);


    private:
    TransportCatalogue& map_;
    std::istream& input_;
    std::vector<std::string> queue_stops;
    std::vector<std::string> queue_buses;
    std::unordered_map <std::string, std::string> queue_lengths;

    void CreateQueue();

    void ParseQueueStops();

    void ParseQueueBuses();

    void ParseQueueLengths();
};