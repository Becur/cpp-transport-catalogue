#include "request_handler.h"

#include "domain.h"

#include <string>
#include <sstream>
#include <optional>

using namespace std::literals;
using std::move;
using json::Node;

RequestHandler::RequestHandler(std::istream& input, std::ostream& output)
: doc(json::Load(input))
, data_make_base(Deserialization(doc.GetRoot().AsDict().at("serialization_settings"s).AsDict().at("file"s).AsString()))
, map_(data_make_base.GetTransportCatalogue())
, router_(data_make_base.GetTransportRoute(map_))
, output_(output){
    if(!doc.GetRoot().AsDict().at("stat_requests"s).AsArray().empty()){
        PrintStat();
    }
}

void RequestHandler::PrintStat(){
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

void RequestHandler::PrintBus(const Node* node){
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

void RequestHandler::PrintStop(const Node* node){
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

void RequestHandler::PrintMap(const Node* node){
    std::ostringstream svg_map;
    MapRenderer map_renderer(map_, data_make_base.GetRenderSettings());
    map_renderer.PrintMap(svg_map);
    array
    .StartDict()
    .Key("request_id"s)
    .Value(node->AsDict().at("id"s).AsInt())
    .Key("map"s)
    .Value(move(svg_map.str()))
    .EndDict();
}

void RequestHandler::PrintRoute(const Node* node){
    const auto& queue = node->AsDict();
    std::optional<DataRoute> route = 
    router_.GetRoute(queue.at("from"s).AsString(), queue.at("to"s).AsString());
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