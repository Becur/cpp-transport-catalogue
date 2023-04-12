#include "json_reader.h"
#include "domain.h"

#include <string>
#include <sstream>
#include <unordered_map>


using namespace std::literals;
using std::move;
using json::Node;

JsonReader::JsonReader(TransportCatalogue& map, std::istream& input, std::ostream& output)
: map_(map)
, doc(json::Load(input))
, output_(output){
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
    std::ostringstream svg_map;
    MapRenderer map_renderer(map_, doc.GetRoot().AsMap().at("render_settings"s).AsMap());
    map_renderer.PrintMap(svg_map);
    json::Dict dict;
    dict["request_id"s] = Node(node->AsMap().at("id"s).AsInt());
    dict["map"s] = Node(move(svg_map.str()));;
    json::Document doc(Node(move(dict)));
    json::Print(move(doc), output_);
}