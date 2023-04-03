#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "transport_catalogue.h"

struct Request{
    bool is_stop;
    std::string name;
};

class StatReader{
    public:
    StatReader(TransportCatalogue& map, std::istream& input = std::cin, std::ostream& output = std::cout);

    private:
    TransportCatalogue& map_;
    std::istream& input_;
    std::ostream& output_;
    std::vector<Request> requests;

    void StartParsRequests();
    void PrintStat();
    void PrintBus(std::string& bus_name);
    void PrintStop(std::string& stop_name);
};