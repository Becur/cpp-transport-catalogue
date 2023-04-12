#pragma once

#include "json.h"
#include "transport_catalogue.h"
#include "geo.h"
#include "map_renderer.h"

#include <iostream>
#include <string_view>
#include <vector>

struct Request{
    int id;
    bool is_stop;
    std::string_view name;
};

class JsonReader{
    public:
    JsonReader(TransportCatalogue& map, std::istream& input = std::cin, std::ostream& output = std::cout);

    private:
    TransportCatalogue& map_;
    const json::Document doc;
    std::ostream& output_;
    const json::Dict& settings;
    std::vector <const json::Node*> queue_stops;
    std::vector <const json::Node*> queue_buses;
    std::vector <std::pair <std::string_view, const json::Node*>> queue_lengths;
    SphereProjector projector;
    svg::Document doc_;
    std::vector<const Bus*> buses_;
    std::vector<svg::Color> color_palette;
    std::vector<const Stop*> stops_;

    void CreateQueue();

    void ParseQueueStops();

    void ParseQueueBuses();

    void ParseQueueLengths();

    void PrintStat();

    void PrintBus(const json::Node* node);

    void PrintStop(const json::Node* node);

    void PrintMap(const json::Node* node);

    void SetSettings();

    void PreparationBuses();

    void ParseColorPalette(const json::Node& node);

    svg::Color ParseColor(const json::Node& node) const;

    void DrawLineBuses();

    void DrawNameBuses();

    void CreateNameBus(svg::Point point, std::string text, svg::Color color);

    void PreparationStops();

    void DrawCircleStops();

    void CreateCircleStop(svg::Point point);

    void DrawNameStops();

    void CreateNameStop(svg::Point point, std::string text);
};