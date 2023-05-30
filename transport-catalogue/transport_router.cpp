#include "transport_router.h"

#include <algorithm>
#include <iterator>

TransportRouter::TransportRouter(
    const TransportCatalogue& catalog, 
    int bus_wait_time, 
    double bus_velocity)
: catalog_(catalog)
, bus_wait_time_(bus_wait_time)
, bus_velocity_(bus_velocity)
, routes_()
, stops_()
, indexes_stops_()
, graph_(CreateGraph())
, router_(graph_){}
    
std::optional<DataRoute> TransportRouter::GetRoute(
    const std::string& start_stop, 
    const std::string& finish_stop){
    auto route = 
    router_.BuildRoute(
        stops_.at(catalog_.GetStop(start_stop)) * 2 + 1, 
        stops_.at(catalog_.GetStop(finish_stop)) * 2 + 1);
    if(route == std::nullopt){
        return std::nullopt;
    }
    if(route.value().edges.size() == 0){
        return DataRoute(0.0, std::vector<std::unique_ptr<RouteElement>>());
    }
    std::vector<std::unique_ptr<RouteElement>> path;
    for(const auto edge_id : route.value().edges){
        const auto& edge = graph_.GetEdge(edge_id);
        if(edge.count == 0){
            path.push_back(std::make_unique<RouteWait>
            (TypeRouteElement::WAIT, 
                edge.weight, 
                indexes_stops_[edge.from / 2]->name_stop));
        }
        else{
            path.push_back(std::make_unique<RouteBus>
            (TypeRouteElement::BUS, 
                edge.weight, 
                routes_.at(edge_id)->name_bus, 
                edge.count));
        }
    }
    return DataRoute(route.value().weight, std::move(path));
}

graph::DirectedWeightedGraph<double> TransportRouter::CreateGraph() {
    CreateStartVertexes();
    graph::DirectedWeightedGraph<double> graph(catalog_.GetStops().size() * 2);
    for(size_t i = 0; i < stops_.size(); ++i){
        graph.AddEdge({i * 2 + 1, i * 2, static_cast<double>(bus_wait_time_), 0});
    }
    for(const Bus& bus : catalog_.GetBuses()){
        GreateEdges(bus.stops.begin(), bus.stops.end(), graph, &bus);
        if(!bus.circular){
            GreateEdges(bus.stops.rbegin(), bus.stops.rend(), graph, &bus);
        }
    }
    return graph;
}

void TransportRouter::CreateStartVertexes(){
    size_t i = 0;
    for(const Stop& stop : catalog_.GetStops()){
        stops_[&stop] = i;
        indexes_stops_[i++] = &stop;
    }
}
