#include "transport_catalogue.h"
#include "geo.h"

#include <algorithm>

using std::move;

void TransportCatalogue::AddStop(Stop&& stop){
    stops_.push_front(move(stop));
    ref_stop[std::string_view(stops_.front().name_stop)] = &stops_.front();
}

void TransportCatalogue::AddBus(std::string&& name_bus, 
std::vector<std::string>&& stops, bool circular){
    std::vector<Stop*> ref_stops;
    ref_stops.reserve(stops.size());
    int count_uniq_stops = 0;
    for(std::string& stop : stops){
        auto ref = ref_stop[std::move(stop)];
        count_uniq_stops += (std::find(ref_stops.begin(), ref_stops.end(), ref) == ref_stops.end()) ? 1 : 0;
        ref_stops.push_back(ref);
    }
    buses_.push_front({move(name_bus), move(ref_stops), circular, count_uniq_stops});
    ref_bus[std::string_view(buses_.front().name_bus)] = &buses_.front();
    std::for_each(buses_.front().stops.begin(), buses_.front().stops.end(),
    [&](Stop* stop){ stop->buses.insert(buses_.front().name_bus);});
}

const Bus* TransportCatalogue::GetBus(const std::string& name_bus) const {
    if(!ref_bus.count(name_bus)){
        return nullptr;
    }
    const Bus* bus = ref_bus.at(name_bus);
    if(bus->length == 0){
        CalculateLength(const_cast<Bus*>(bus));
        CalculatePath(const_cast<Bus*>(bus));
    }
    return bus;
}

void TransportCatalogue::CalculateLength(Bus* bus) const{
    for(size_t i = 1; i < bus->stops.size(); ++i){
        bus->length += geo::ComputeDistance(geo::Coordinates({bus->stops[i - 1]->latitude, bus->stops[i - 1]->longitude}), 
        geo::Coordinates({bus->stops[i]->latitude, bus->stops[i]->longitude}));
    }
    bus->length *= (bus->circular) ? 1 : 2;
}

const Stop* TransportCatalogue::GetStop(const std::string& name_stop) const{
    return (ref_stop.count(name_stop)) ? ref_stop.at(name_stop) : nullptr;
}

size_t TransportCatalogue::HasherRefsStop::operator()(const std::pair<const Stop*, const Stop*> pair) const{
    size_t first_stop = reinterpret_cast<size_t>(pair.first);
    size_t second_stop = reinterpret_cast<size_t>(pair.second);
    return first_stop * 37 + second_stop;
}

void TransportCatalogue::SetLengths(std::string&& name_stop,  std::unordered_map<std::string, int>&& length_to_stop){
    for(const auto& [to_stop, length] :  move(length_to_stop)){
        lengths[{ref_stop[name_stop], ref_stop[to_stop]}] = length;
    }
}

void TransportCatalogue::CalculatePath(Bus* bus) const{
    for(size_t i = 1; i < bus->stops.size(); ++i){
        bus->path += GetLength(bus->stops[i - 1], bus->stops[i]);
    }
    if(!bus->circular){
        for(int i = bus->stops.size() - 1; i > 0; --i){
            bus->path += GetLength(bus->stops[i], bus->stops[i - 1]);
        }
    }
}

int TransportCatalogue::GetLength(const Stop* lstop, const Stop* rstop) const{
    return (lengths.count({lstop, rstop}))
            ? lengths.at({lstop, rstop})
            : lengths.at({rstop, lstop});
}

const std::deque<Bus>& TransportCatalogue::GetBuses() const{
    return buses_;
}

const std::deque<Stop>& TransportCatalogue::GetStops() const{
    return stops_;
}