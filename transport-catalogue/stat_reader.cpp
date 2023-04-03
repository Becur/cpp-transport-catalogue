#include "stat_reader.h"

#include <limits>
#include <iomanip>
#include <string_view>

using std::string;
using namespace std::string_literals;

StatReader::StatReader(TransportCatalogue& map, std::istream& input, std::ostream& output)
: map_(map)
, input_(input)
, output_(output){
    StartParsRequests();
    PrintStat();
}

void StatReader::StartParsRequests(){
    int num_request;
    input_ >> num_request;
    input_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    requests.reserve(num_request);
    for(int i = 0; i < num_request; ++i){
        string line;
        std::getline(input_, line);
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
    output_ << "Bus "s << bus_name << ": "s;
    if(bus == nullptr) { 
        output_ << "not found\n";
    }
    else{
    output_ << (bus->stops.size() + ((bus->circular) ? 0 : (bus->stops.size() - 1)))
    << " stops on route, "s << bus->count_unique_stop << " unique stops, "s 
    << bus->path << " route length, "s << std::setprecision(6)
    << bus->path / bus->length << " curvature\n"s;
    }
}

void StatReader::PrintStop(std::string& stop_name){
    const Stop* stop = map_.GetStop(stop_name);
    output_ << "Stop "s << stop_name <<": "s;
    if(stop == nullptr){
        output_ << "not found"s;
    }
    else if (stop->buses.empty()){
        output_ << "no buses"s;
    }
    else{
        output_ << "buses";
        for(const std::string_view name_bus :  stop->buses){
            output_ << ' ' << name_bus;
        }
    }
    output_ << '\n';
}