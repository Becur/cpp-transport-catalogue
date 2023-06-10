#pragma once 

#include "json.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"
#include "deserialization.h"

#include <iostream>
#include <vector>

class RequestHandler{
public:
    RequestHandler(std::istream& input = std::cin, std::ostream& output = std::cout);

private:
    const json::Document doc;
    Deserialization data_make_base;
    const TransportCatalogue& map_;
    TransportRouter router_;
    std::ostream& output_;
    json::Builder array = json::Builder{};

    void PrintStat();

    void PrintBus(const json::Node* node);

    void PrintStop(const json::Node* node);

    void PrintMap(const json::Node* node);

    void PrintRoute(const json::Node* node);
};