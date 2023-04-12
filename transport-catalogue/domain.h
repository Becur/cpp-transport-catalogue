#pragma once

#include <string>
#include <set>
#include <vector>


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
    double length = -1;
    int path = 0;
};