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
    if(!doc.GetRoot().AsDict().at("stat_requests"s).AsArray().empty()){
        PrintStat();
    }
}

void JsonReader::CreateQueue(){
    const auto& base_requests = doc.GetRoot().AsDict().at("base_requests"s);
    for(const auto& node : base_requests.AsArray()){
        (node.AsDict().at("type"s).AsString() == "Stop"s)
        ? queue_stops.push_back(&node)
        : queue_buses.push_back(&node);
    }
}

void JsonReader::ParseQueueStops(){
    for(const Node* node : queue_stops){
        const auto& stop_js = node->AsDict();
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
        const auto& bus = node->AsDict();
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
        for(const auto& [string, node] : node->AsDict()){
            length_to_stop[string] = node.AsInt();
        }
        map_.SetLengths(std::string(name), move(length_to_stop));
    }
}

void JsonReader::PrintStat(){
    const auto& settings = doc.GetRoot().AsDict().at("routing_settings"s).AsDict();
    router_.emplace(
        map_,
        settings.at("bus_wait_time"s).AsInt(),
        settings.at("bus_velocity"s).AsDouble());
    const auto& stat_requests = doc.GetRoot().AsDict().at("stat_requests"s);
    array.StartArray();
    for(const auto& node : stat_requests.AsArray()){
        if(node.AsDict().at("type"s).AsString() == "Bus"){
            PrintBus(&node);
        } 
        else if(node.AsDict().at("type"s).AsString() == "Stop"s){
            PrintStop(&node);
        }
        else if(node.AsDict().at("type"s).AsString() == "Map"s){
            PrintMap(&node);
        }
        else{
            PrintRoute(&node);
        }
    }
    json::Print(json::Document(array.EndArray().Build()), output_);
}

void JsonReader::PrintBus(const Node* node){
    const Bus* bus = map_.GetBus(node->AsDict().at("name"s).AsString());
    array
    .StartDict()
    .Key("request_id"s)
    .Value(node->AsDict().at("id"s).AsInt());
    if(bus == nullptr) {
        array
        .Key("error_message"s)
        .Value("not found"s)
        .EndDict();
    }
    else{
        array
        .Key("stop_count"s)
        .Value(static_cast<int>(bus->stops.size() 
        + ((bus->circular) ? 0 : (bus->stops.size() - 1))))
        .Key("unique_stop_count"s)
        .Value(bus->count_unique_stop)
        .Key("route_length"s)
        .Value(bus->path)
        .Key("curvature"s)
        .Value(bus->path / bus->length)
        .EndDict();
    }
}

void JsonReader::PrintStop(const Node* node){
    const Stop* stop = map_.GetStop(node->AsDict().at("name"s).AsString());
    array
    .StartDict()
    .Key("request_id"s)
    .Value(node->AsDict().at("id"s).AsInt());
    if(stop == nullptr) { 
        array
        .Key("error_message"s)
        .Value("not found"s)
        .EndDict();
    }
    else{
        array
        .Key("buses"s)
        .StartArray();
        for(const auto name_bus : stop->buses){
            array.Value(std::string(name_bus));
        }
        array
        .EndArray()
        .EndDict();
    }
}

void JsonReader::PrintMap(const Node* node){
    std::ostringstream svg_map;
    MapRenderer map_renderer(map_, doc.GetRoot().AsDict().at("render_settings"s).AsDict());
    map_renderer.PrintMap(svg_map);
    array
    .StartDict()
    .Key("request_id"s)
    .Value(node->AsDict().at("id"s).AsInt())
    .Key("map"s)
    .Value(move(svg_map.str()))
    .EndDict();
}

void JsonReader::PrintRoute(const Node* node){
    const auto& queue = node->AsDict();
    std::optional<DataRoute> route = 
    router_.value().GetRoute(queue.at("from"s).AsString(), queue.at("to"s).AsString());
    array
    .StartDict()
    .Key("request_id"s)
    .Value(queue.at("id"s).AsInt());
    if(route == std::nullopt){
        array
        .Key("error_message"s)
        .Value("not found"s)
        .EndDict();
    }
    else{
        array
        .Key("total_time"s)
        .Value(route.value().time_)
        .Key("items"s)
        .StartArray();
        for(size_t i = 0; i < route.value().path_.size(); ++i){
            array
            .StartDict()
            .Key("time"s)
            .Value(route.value().path_[i]->time_);
            if(route.value().path_[i]->type_ == TypeRouteElement::WAIT){
                const RouteWait* wait = dynamic_cast<const RouteWait*>(route.value().path_[i].release());
                array
                .Key("type"s)
                .Value("Wait"s)
                .Key("stop_name"s)
                .Value(wait->name_stop_)
                .EndDict();
                delete wait;
            }
            else{
                const RouteBus* bus = dynamic_cast<const RouteBus*>(route.value().path_[i].release());
                array
                .Key("type"s)
                .Value("Bus"s)
                .Key("bus"s)
                .Value(std::string(bus->name_bus_))
                .Key("span_count"s)
                .Value(bus->span_count_)
                .EndDict();
                delete bus;
            }
        }
        array
        .EndArray().EndDict();
    }

}