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

TransportRouter::TransportRouter(const TransportCatalogue& catalog, SaveTransportRouter&& save)
: catalog_(catalog)
, bus_wait_time_(save.bus_wait_time)
, bus_velocity_(save.bus_velocity)
, graph_(std::move(save.edges), std::move(save.incidence_lists))
, router_(std::move(save.routes_internal_data), graph_){
    for(auto& [id, name_bus] : save.routes){
        routes_.insert({id, catalog_.GetBus(std::move(name_bus))});
    }
    for(auto& [id, name_stop] : save.indexes_stops){
        const Stop* stop = catalog_.GetStop(std::move(name_stop));
        stops_.insert({stop, id});
        indexes_stops_.insert({id, stop});
    }
}
    
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
        CreateEdges(bus.stops.begin(), bus.stops.end(), graph, &bus);
        if(!bus.circular){
            CreateEdges(bus.stops.rbegin(), bus.stops.rend(), graph, &bus);
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

std::pair<std::vector<graph::Edge<double>>, std::vector<std::vector<graph::EdgeId>>> TransportRouter::SaveGraph(){
    return graph_.Save();
}

graph::Router<double>::RoutesInternalData TransportRouter::SaveRouter(){
    return router_.Save();
}

std::pair<std::unordered_map<graph::EdgeId, std::string>, std::unordered_map<size_t, std::string>> TransportRouter::Save() const{
    std::unordered_map<graph::EdgeId, std::string> edge_id_bus_name;
    for(const auto& [id, bus] : routes_){
        edge_id_bus_name.insert({id, bus->name_bus});
    }
    std::unordered_map<size_t, std::string> id_stop_name;
    for(const auto& [id, stop] : indexes_stops_){
        id_stop_name.insert({id, stop->name_stop});
    }
    return {std::move(edge_id_bus_name), std::move(id_stop_name)};
}
