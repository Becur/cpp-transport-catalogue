#pragma once

#include "domain.h"

#include <unordered_map>
#include <deque>
#include <string>
#include <vector>

class TransportCatalogue{
    public:
    void AddStop(Stop stop);

    void AddBus(std::string name_bus, std::vector<std::string> stops, bool circular);

    const Bus* GetBus(const std::string& name_bus);

    const Stop* GetStop(const std::string& name_stop) const;

    void SetLengths(const std::string& name_stop, const std::unordered_map<std::string, int>& length_to_stop);

    int GetLength(const Stop* lstop, const Stop* rstop) const;

    const std::deque<Bus>& GetBuses() const;
    const std::deque<Stop>& GetStops() const;
    private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> ref_stop;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> ref_bus;

    class HasherRefsStop{
        public:
        size_t operator()(const std::pair<const Stop*, const Stop*> pair) const;
    };
    
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, HasherRefsStop> lengths;

    void CalculateLength(Bus* bus);

    void CalculatePath(Bus* bus);
};