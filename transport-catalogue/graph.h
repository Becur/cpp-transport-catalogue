#pragma once

#include "ranges.h"

#include <cstdlib>
#include <vector>

namespace graph {

using VertexId = size_t;
using EdgeId = size_t;

template <typename Weight>
struct Edge {
    VertexId from;
    VertexId to;
    Weight weight;
    int count;
};

template <typename Weight>
class DirectedWeightedGraph {
private:
    using IncidenceList = std::vector<EdgeId>;
    using IncidentEdgesRange = ranges::Range<typename IncidenceList::const_iterator>;

public:
    DirectedWeightedGraph() = default;
    explicit DirectedWeightedGraph(size_t vertex_count);
    DirectedWeightedGraph(std::vector<Edge<Weight>>&& edges,std::vector<IncidenceList>&& incidence_lists);
    EdgeId AddEdge(Edge<Weight>&& edge); // lvalue

    size_t GetVertexCount() const;
    size_t GetEdgeCount() const;
    const Edge<Weight>& GetEdge(EdgeId edge_id) const;
    IncidentEdgesRange GetIncidentEdges(VertexId vertex) const;

    std::pair<std::vector<Edge<Weight>>, std::vector<IncidenceList>> Save();
private:
    std::vector<Edge<Weight>> edges_;
    std::vector<IncidenceList> incidence_lists_;
};

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(size_t vertex_count)
    : incidence_lists_(vertex_count) {
}

template <typename Weight>
DirectedWeightedGraph<Weight>::DirectedWeightedGraph(std::vector<Edge<Weight>>&& edges,std::vector<IncidenceList>&& incidence_lists)
: edges_(std::move(edges)), incidence_lists_(std::move(incidence_lists)) {}

template <typename Weight>
EdgeId DirectedWeightedGraph<Weight>::AddEdge(Edge<Weight>&& edge) {
    edges_.push_back(std::move(edge));
    const EdgeId id = edges_.size() - 1;
    incidence_lists_.at(edges_.back().from).push_back(id);
    return id;
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetVertexCount() const {
    return incidence_lists_.size();
}

template <typename Weight>
size_t DirectedWeightedGraph<Weight>::GetEdgeCount() const {
    return edges_.size();
}

template <typename Weight>
const Edge<Weight>& DirectedWeightedGraph<Weight>::GetEdge(EdgeId edge_id) const {
    return edges_.at(edge_id);
}

template <typename Weight>
typename DirectedWeightedGraph<Weight>::IncidentEdgesRange
DirectedWeightedGraph<Weight>::GetIncidentEdges(VertexId vertex) const {
    return ranges::AsRange(incidence_lists_.at(vertex));
}

template <typename Weight>
std::pair<std::vector<Edge<Weight>>, std::vector<std::vector<EdgeId>>> DirectedWeightedGraph<Weight>::Save(){
    return {std::move(edges_), std::move(incidence_lists_)};
}
}  // namespace graph