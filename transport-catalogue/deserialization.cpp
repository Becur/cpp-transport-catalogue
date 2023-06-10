#include "deserialization.h"

#include <fstream>
#include <utility>
#include <vector>
#include <string>
#include <unordered_map>

Deserialization::Deserialization(const std::string& path_file){
    std::ifstream input(path_file, std::ios::in | std::ios::binary);
    obj_.ParseFromIstream(&input);
}

TransportCatalogue Deserialization::GetTransportCatalogue(){
    AddStops();
    AddBus();
    AddLenghts();
    return std::move(new_catalog);
}

TransportCatalogue Deserialization::GetTransportCatalogue(const transport::Catalogue& catalog){
    *obj_.mutable_catalogue() = catalog;
    return GetTransportCatalogue();
}

void Deserialization::AddStops(){
    for(int i = 0; i < obj_.catalogue().stop_size(); ++i){
        transport::Stop* stop = obj_.mutable_catalogue()->mutable_stop(i);
        Stop new_stop;
        new_stop.name_stop = std::move(*stop->mutable_name());
        new_stop.latitude = stop->latitude();
        new_stop.longitude = stop->longitude();
        new_catalog.AddStop(std::move(new_stop));
    }
}

void Deserialization::AddBus(){
    for(int i = 0; i < obj_.catalogue().bus_size(); ++i){
        transport::Bus* bus = obj_.mutable_catalogue()->mutable_bus(i);
        std::vector<std::string> stops;
        stops.reserve(bus->name_stop_size());
        for(int j = 0; j < bus->name_stop_size(); ++j){
            stops.push_back(std::move(*bus->mutable_name_stop(j)));
        }
        new_catalog.AddBus(std::move(*bus->mutable_name()), std::move(stops), bus->circular());
    }
}

void Deserialization::AddLenghts(){
    for(int i = 0; i < obj_.catalogue().lenghts_size(); ++i){
        transport::Lenghts* lenghts = obj_.mutable_catalogue()->mutable_lenghts(i);
        std::unordered_map<std::string, int> length_to_stop;
        for(int j = 0; j < lenghts->other_stop_size(); ++j){
            length_to_stop.insert({std::move(*lenghts->mutable_other_stop(j)), lenghts->lenght_to_other_stop(j)});
        }
        new_catalog.SetLengths(std::move(*lenghts->mutable_name_stop()), std::move(length_to_stop));
    }
}

const render_settings::Settings& Deserialization::GetRenderSettings() const{
    return obj_.settings();
}

TransportRouter Deserialization::GetTransportRoute(const TransportCatalogue& catalog){
    AddGraphData();
    AddRouteData();
    AddTransportRouteData();
    return TransportRouter(catalog, std::move(save));
}

void Deserialization::AddGraphData(){
    const save::Graph& graph = obj_.graph();
    std::vector<graph::Edge<double>> edges;
    edges.reserve(graph.edge_size());
    for(const save::Edge& edge : graph.edge()){
        graph::Edge<double> new_edge;
        new_edge.from = edge.from();
        new_edge.to = edge.to();
        new_edge.weight = edge.weight();
        new_edge.count = edge.count();
        edges.push_back(std::move(new_edge));
    }
    save.edges = std::move(edges);
    std::vector<std::vector<graph::EdgeId>> incidence_lists;
    incidence_lists.reserve(graph.size_list_size());
    size_t k = 0;
    for(size_t i = 0; i < graph.size_list_size(); ++i){
        incidence_lists.push_back(std::vector<graph::EdgeId>(graph.size_list(i)));
        for(int j = 0; j < graph.size_list(i); ++j){
            incidence_lists[i][j] = graph.incidence_lists(k++);
        }
    }
    save.incidence_lists = std::move(incidence_lists);
}

void Deserialization::AddRouteData(){
    const save::RoutesInternalData& data = obj_.routes_internal_data();
    std::vector<std::vector<std::optional<graph::RouteInternalData<double>>>> new_data;
    new_data.reserve(data.sizes_data_size());
    size_t k = 0;
    for(size_t i = 0; i < data.sizes_data_size(); ++i){
        new_data.push_back(std::vector<std::optional<graph::RouteInternalData<double>>>(data.sizes_data(i)));
        for(int j = 0; j < data.sizes_data(i); ++j){
            if(data.option(k).has_value()){
                new_data[i][j] = graph::RouteInternalData<double>();
                new_data[i][j].value().weight = data.option(k).value().weight();
                if(data.option(k).value().has_prev_edge()){
                    new_data[i][j].value().prev_edge = data.option(k).value().prev_edge().edge_id();
                }
            }
            ++k;
        }
    }
    save.routes_internal_data = std::move(new_data);
}

void Deserialization::AddTransportRouteData(){
    save::TransportRoute& route = *obj_.mutable_transport_route();
    save.bus_wait_time = route.bus_wait_time();
    save.bus_velocity = route.bus_velocity();
    std::unordered_map<graph::EdgeId, std::string> routes;
    for(size_t i = 0; i < route.edge_id_size(); ++i){
        routes.insert({route.edge_id(i), std::move(*route.mutable_name_bus(i))});
    }
    save.routes = std::move(routes);
    std::unordered_map<size_t, std::string> indexes_stops;
    for(size_t i = 0; i < route.stop_id_size(); ++i){
        indexes_stops.insert({route.stop_id(i), std::move(*route.mutable_name_stop(i))});
    }
    save.indexes_stops = std::move(indexes_stops);
}