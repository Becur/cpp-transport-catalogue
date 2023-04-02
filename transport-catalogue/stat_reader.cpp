#include "stat_reader.h"

#include <iostream>
#include <limits>
#include <iomanip>
#include <string_view>

using std::cin;
using std::string;
using namespace std::string_literals;
using std::cout;

StatReader::StatReader(TransportCatalogue& map)
: map_(map){
    StartParsRequests();
    PrintStat();
}

void StatReader::StartParsRequests(){
    int num_request;
    cin >> num_request;
    cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    requests.reserve(num_request);
    for(int i = 0; i < num_request; ++i){
        string line;
        std::getline(cin, line);
        int first_ch = line.find_first_not_of(' ');
        int last_ch = line.find_first_of(' ', first_ch);
        string word = line.substr(first_ch, last_ch - first_ch);
        bool is_stop = (word == "Stop"s);
        first_ch = line.find_first_not_of(' ', last_ch);
        word = line.substr(first_ch, line.find_last_not_of(' ') - first_ch + 1);
        requests.push_back({is_stop, std::move(word)});
    }
}

void StatReader::PrintStat(){
    for(auto& request : requests){
        (request.is_stop) ? PrintStop(request.name) : PrintBus(request.name);
    }
}

void StatReader::PrintBus(string& bus_name){
    const Bus* bus = map_.GetBus(bus_name);
    (bus == nullptr) ? cout << "Bus "s << bus_name << ": not found\n"
    : cout << "Bus "s << bus->name_bus << ": "
    << (bus->stops.size() + ((bus->circular) ? 0 : (bus->stops.size() - 1)))
    << " stops on route, "s << bus->count_unique_stop << " unique stops, "s 
    << bus->path << " route length, "s << std::setprecision(6)
    << bus->path / bus->lenght << " curvature\n"s;
}

void StatReader::PrintStop(std::string& stop_name){
    const Stop* stop = map_.GetStop(stop_name);
    cout << "Stop "s << stop_name <<": "s;
    if(stop == nullptr){
        cout << "not found"s;
    }
    else if (stop->buses.empty()){
        cout << "no buses"s;
    }
    else{
        cout << "buses";
        for(const std::string_view name_bus :  stop->buses){
            cout << ' ' << name_bus;
        }
    }
    cout << '\n';
}