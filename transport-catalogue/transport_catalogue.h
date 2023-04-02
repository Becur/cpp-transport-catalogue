#pragma once

#include <vector>
#include <string>
#include <deque>
#include <unordered_map>
#include <string_view>
#include <set>

#include "geo.h"

struct Stop{
    std::string name_stop;
    double latitude;
    double longitude;
    std::set<std::string_view, std::less<>> buses;
};

struct Bus{
    std::string name_bus;
    std::vector<Stop*> stops;
    bool circular;
    int count_unique_stop = 0;
    double lenght = -1;
    int path = 0;
};

class TransportCatalogue{
    public:
    void AddStop(Stop stop);

    void AddBus(std::string name_bus, std::vector<std::string> stops, bool circular);

    const Bus* GetBus(const std::string& name_bus);

    const Stop* GetStop(const std::string& name_stop) const;

    void SetLenghts(std::string name_stop, std::unordered_map<std::string, int> lenght_to_stop);

    int GetLenght(const Stop* lstop, const Stop* rstop) const;
    
    private:
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, Stop*> ref_stop;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, Bus*> ref_bus;

    class HasherRefsStop{
        public:
        size_t operator()(const std::pair<const Stop*, const Stop*> pair) const;
    };
    
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, HasherRefsStop> lenghts;

    void CalculationLenght(Bus* bus);

    void CalculationPath(Bus* bus);
};