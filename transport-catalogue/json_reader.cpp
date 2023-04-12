#include "json_reader.h"
#include "domain.h"

#include <string>
#include <algorithm>
#include <sstream>


using namespace std::literals;
using std::move;
using json::Node;
using svg::Color;

JsonReader::JsonReader(TransportCatalogue& map, std::istream& input, std::ostream& output)
: map_(map)
, doc(json::Load(input))
, output_(output)
, settings(doc.GetRoot().AsMap().at("render_settings"s).AsMap()){
    CreateQueue();
    ParseQueueStops();
    ParseQueueBuses();
    ParseQueueLengths();
    if(!doc.GetRoot().AsMap().at("stat_requests"s).AsArray().empty()){
        PrintStat();
    }
}

void JsonReader::CreateQueue(){
    const auto& base_requests = doc.GetRoot().AsMap().at("base_requests"s);
    for(const auto& node : base_requests.AsArray()){
        (node.AsMap().at("type"s).AsString() == "Stop"s)
        ? queue_stops.push_back(&node)
        : queue_buses.push_back(&node);
}
}

void JsonReader::ParseQueueStops(){
    for(const Node* node : queue_stops){
        const auto& stop_js = node->AsMap();
        queue_lengths.push_back({stop_js.at("name"s).AsString(), &stop_js.at("road_distances"s)});
        Stop stop;
        stop.name_stop = stop_js.at("name"s).AsString();
        stop.latitude = stop_js.at("latitude"s).AsDouble();
        stop.longitude = stop_js.at("longitude"s).AsDouble();
        map_.AddStop(move(stop));
    }
}

void JsonReader::ParseQueueBuses(){
    for(const Node* node : queue_buses){
        const auto& bus = node->AsMap();
        const auto& stops_js = bus.at("stops"s).AsArray();
        std::vector<std::string> stops(stops_js.size());
        for(size_t i = 0; i < stops.size(); ++i){
            stops[i] = stops_js[i].AsString();
        }
        map_.AddBus(bus.at("name"s).AsString(), move(stops), bus.at("is_roundtrip"s).AsBool());
    }
}

void JsonReader::ParseQueueLengths(){
    for(const auto& [name, node] : queue_lengths){
        std::unordered_map<std::string, int> length_to_stop;
        for(const auto& [string, node] : node->AsMap()){
            length_to_stop[string] = node.AsInt();
        }
        map_.SetLengths(std::string(name), move(length_to_stop));
    }
}

void JsonReader::PrintStat(){
    const auto& stat_requests = doc.GetRoot().AsMap().at("stat_requests"s);
    output_ << "["s;
    bool is_first = true;
    for(const auto& node : stat_requests.AsArray()){
       if(is_first){
            is_first = false;
        }
        else{
            output_ << ","s;
        }
        if(node.AsMap().at("type"s).AsString() == "Bus"){
            PrintBus(&node);
        } 
        else if(node.AsMap().at("type"s).AsString() == "Stop"){
            PrintStop(&node);
        }
        else{
            PrintMap(&node);
        }
    }
    output_ << "]"s;
}

void JsonReader::PrintBus(const Node* node){
    const Bus* bus = map_.GetBus(node->AsMap().at("name"s).AsString());
    json::Dict res_node;
    res_node["request_id"s] = Node(node->AsMap().at("id"s).AsInt());
    if(bus == nullptr) { 
        res_node["error_message"s] = Node("not found"s);
    }
    else{
        res_node["stop_count"s] = Node(static_cast<int>(bus->stops.size() 
        + ((bus->circular) ? 0 : (bus->stops.size() - 1))));
        res_node["unique_stop_count"s] = Node(bus->count_unique_stop);
        res_node["route_length"s] = Node(bus->path);
        res_node["curvature"s] = Node(bus->path / bus->length);
    }
    json::Document doc(move(res_node));
    json::Print(move(doc), output_);
}

void JsonReader::PrintStop(const Node* node){
    const Stop* stop = map_.GetStop(node->AsMap().at("name"s).AsString());
    json::Dict res_node;
    res_node["request_id"s] = Node(node->AsMap().at("id"s).AsInt());
    if(stop == nullptr) { 
        res_node["error_message"s] = Node("not found"s);
    }
    else{
        json::Array buses;
        buses.reserve(stop->buses.size());
        for(const auto name_bus : stop->buses){
            buses.push_back(Node(std::string(name_bus)));
        }
        res_node["buses"s] = Node(move(buses));
    }
    json::Document doc(move(res_node));
    json::Print(move(doc), output_);
}

void JsonReader::PrintMap(const Node* node){
    if((!settings.empty()) && (doc_.Empty())){
        SetSettings();
        PreparationBuses();
        DrawLineBuses();
        DrawNameBuses();
        PreparationStops();
        DrawCircleStops();
        DrawNameStops();
    }
    std::ostringstream svg_map;
    doc_.Render(svg_map);
    json::Dict dict;
    dict["request_id"s] = Node(node->AsMap().at("id"s).AsInt());
    dict["map"s] = Node(move(svg_map.str()));;
    json::Document doc(Node(move(dict)));
    json::Print(move(doc), output_);
}

void JsonReader::SetSettings(){
    std::vector <geo::Coordinates> coordinates;
    coordinates.reserve(map_.GetStops().size());
    for(const auto& stop : map_.GetStops()){
        if(!stop.buses.empty()){
            coordinates.push_back({stop.latitude, stop.longitude});
        }
    }
    projector = SphereProjector(coordinates.begin(), coordinates.end(),
    settings.at("width"s).AsDouble(), settings.at("height"s).AsDouble(),
    settings.at("padding"s).AsDouble());

}

void JsonReader::PreparationBuses(){
    buses_.reserve(map_.GetBuses().size());
    for(const Bus& bus : map_.GetBuses()){
        buses_.push_back(&bus);
    }
    std::sort(buses_.begin(), buses_.end(), 
    [](const Bus* lbus, const Bus* rbus){
        return lbus->name_bus < rbus->name_bus;
    });
}

void JsonReader::DrawLineBuses(){
    ParseColorPalette(settings.at("color_palette"s));
    int i = 0;
    for(const auto* bus : buses_){
        if(bus->stops.empty()){
            continue;
        }
        svg::Polyline polyline;
        polyline
        .SetStrokeWidth(settings.at("line_width"s).AsDouble())
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
        .SetFillColor(svg::NoneColor)
        .SetStrokeColor(color_palette[i++ % color_palette.size()]);
        for(const auto* stop : bus->stops){
            polyline.AddPoint(projector({stop->latitude, stop->longitude}));
        }
        if(!bus->circular){
            bool is_first = true;
            std::for_each(bus->stops.rbegin(), bus->stops.rend(),
            [&](const auto* stop){
                if(is_first){
                    is_first = false;
                }
                else{
                    polyline.AddPoint(projector({stop->latitude, stop->longitude}));
                }
            });
        }
        doc_.Add(move(polyline));
    }
}

void JsonReader::ParseColorPalette(const Node& node){
    color_palette.reserve(node.AsArray().size());
    for(const auto& color : node.AsArray()){
        color_palette.push_back(ParseColor(color));
    }
}

Color JsonReader::ParseColor(const Node& node) const{
    if(node.IsString()){
        return Color(node.AsString());
    }
    else{
        const auto& nums = node.AsArray();
        if(nums.size() == 3){
            return Color(svg::Rgb(nums[0].AsInt(),
            nums[1].AsInt(), nums[2].AsInt()));
        }
        else{
            return Color(svg::Rgba(nums[0].AsInt(),
            nums[1].AsInt(), nums[2].AsInt(), nums[3].AsDouble()));
        }
    }
}

void JsonReader::DrawNameBuses(){
    int i = 0;
    for(const auto* bus : buses_){
        CreateNameBus(projector({bus->stops[0]->latitude, 
        bus->stops[0]->longitude}), bus->name_bus, 
        color_palette[i % color_palette.size()]);
        if((!bus->circular) && (bus->stops[0] != bus->stops.back())){
            CreateNameBus(projector({bus->stops.back()->latitude, 
            bus->stops.back()->longitude}), bus->name_bus, 
            color_palette[i % color_palette.size()]);
        }
        ++i;
    }
}

void JsonReader::CreateNameBus(svg::Point point, std::string name_bus, Color color){
    svg::Text name;
    name
    .SetPosition(point)
    .SetOffset({settings.at("bus_label_offset"s).AsArray()[0].AsDouble(),
    settings.at("bus_label_offset"s).AsArray()[1].AsDouble()})
    .SetFontSize(settings.at("bus_label_font_size"s).AsInt())
    .SetFontFamily("Verdana"s)
    .SetFontWeight("bold"s)
    .SetData(name_bus);

    svg::Text base;
    base = name;

    name
    .SetFillColor(color);

    Color under_color = ParseColor(settings.at("underlayer_color"s));
    base
    .SetFillColor(under_color)
    .SetStrokeColor(move(under_color))
    .SetStrokeWidth(settings.at("underlayer_width"s).AsDouble())
    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc_.Add(move(base));
    doc_.Add(move(name));
}

void JsonReader::PreparationStops(){
    stops_.reserve(map_.GetStops().size());
    for(const Stop& stop : map_.GetStops()){
        stops_.push_back(&stop);
    }
    std::sort(stops_.begin(), stops_.end(), 
    [](const Stop* lbus, const Stop* rbus){
        return lbus->name_stop < rbus->name_stop;
    });
}


void JsonReader::DrawCircleStops(){
    for(const Stop* stop : stops_){
        if(!stop->buses.empty()){
            CreateCircleStop(projector({stop->latitude, stop->longitude}));
        }
    }
}

void JsonReader::CreateCircleStop(svg::Point point){
    svg::Circle circle;
    circle
    .SetCenter(point)
    .SetFillColor(Color("white"s))
    .SetRadius(settings.at("stop_radius"s).AsDouble());
    doc_.Add(move(circle));
}

void JsonReader::DrawNameStops(){
    for(const auto* stop : stops_){
        if(!stop->buses.empty()){
            CreateNameStop(projector({stop->latitude, stop->longitude}), stop->name_stop);
        }
    }
}

void JsonReader::CreateNameStop(svg::Point point, std::string name_stop){
    svg::Text name;
    name
    .SetPosition(point)
    .SetOffset({settings.at("stop_label_offset"s).AsArray()[0].AsDouble(),
    settings.at("stop_label_offset"s).AsArray()[1].AsDouble()})
    .SetFontSize(settings.at("stop_label_font_size"s).AsInt())
    .SetFontFamily("Verdana"s)
    .SetData(name_stop);

    svg::Text base;
    base = name;

    name
    .SetFillColor(Color("black"s));

    Color under_color = ParseColor(settings.at("underlayer_color"s));
    base
    .SetFillColor(under_color)
    .SetStrokeColor(move(under_color))
    .SetStrokeWidth(settings.at("underlayer_width"s).AsDouble())
    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc_.Add(move(base));
    doc_.Add(move(name));
}