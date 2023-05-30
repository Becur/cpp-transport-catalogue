#pragma once

#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"
#include "domain.h"

#include <vector>
#include <unordered_map>
#include <optional>
#include <memory>

enum class TypeRouteElement{
    WAIT,
    BUS
};

struct RouteElement{
    TypeRouteElement type_;
    double time_;

    RouteElement(TypeRouteElement type, double time)
    : type_(type), time_(time){}

    virtual ~RouteElement() = default;
};

struct RouteBus : public RouteElement{
    const std::string& name_bus_;
    int span_count_;

    RouteBus(TypeRouteElement type, double time, const std::string& name_bus, int span_count)
    : RouteElement(type, time), name_bus_(name_bus), span_count_(span_count){}

    ~RouteBus() override = default;
};

struct RouteWait : public RouteElement{
    const std::string& name_stop_;

    RouteWait(TypeRouteElement type, double time, const std::string& name_stop)
    : RouteElement(type, time), name_stop_(name_stop){}
    
    ~RouteWait() override = default;
};

struct DataRoute{
    double time_;
    std::vector<std::unique_ptr<RouteElement>> path_;

    DataRoute(double time, std::vector<std::unique_ptr<RouteElement>>&& path)
    : time_(time), path_(std::move(path)){}
};

class TransportRouter{
public:
    TransportRouter(const TransportCatalogue& catalog, int bus_wait_time, double bus_velocity);
    
    std::optional<DataRoute> GetRoute(const std::string& start_stop, const std::string& finish_stop);
private:
    const TransportCatalogue& catalog_;
    int bus_wait_time_;
    double bus_velocity_;
    std::unordered_map<graph::EdgeId, const Bus*> routes_;
    std::unordered_map<const Stop*, size_t> stops_;
    std::unordered_map<size_t, const Stop*> indexes_stops_;
    graph::DirectedWeightedGraph<double> graph_;
    graph::Router<double> router_;

    graph::DirectedWeightedGraph<double> CreateGraph();
    void CreateStartVertexes();

    template<typename InputIt>
    void GreateEdges(InputIt start_stop, InputIt finish_stop, 
    graph::DirectedWeightedGraph<double>& graph, const Bus* bus);
};

template<typename InputIt>
void TransportRouter::GreateEdges(InputIt start_stop, InputIt finish_stop, 
graph::DirectedWeightedGraph<double>& graph, const Bus* bus){
    size_t size_container = std::distance(start_stop, finish_stop);
    std::vector<std::vector<double>> edges_weight(size_container - 1, std::vector<double>(size_container, 0.0));
    std::vector<size_t> stops;
    stops.reserve(size_container);
    size_t i = 0;
    for(auto it = start_stop; it != std::next(finish_stop, -1); it = std::next(it)){
        stops.push_back(stops_.at(*it));
        double time = catalog_.GetLength(*it, *std::next(it)) / (bus_velocity_ * 1000 / 60);
        for(size_t j = 0; j <= i; ++j){
            for(size_t k = 1 + i; k < size_container; ++k){
                edges_weight[j][k] += time;
            }
        }
        ++i;
    }
    stops.push_back(stops_.at(*std::next(finish_stop, -1)));
    for(size_t j = 0; j < size_container - 1; ++j){
        for(size_t k = j + 1; k < size_container; ++k){
            graph::EdgeId id = graph.AddEdge(graph::Edge<double>{stops[j] * 2, stops[k] * 2 + 1, edges_weight[j][k], static_cast<int>(k - j)});
            routes_[id] = bus;
        }
    }
}