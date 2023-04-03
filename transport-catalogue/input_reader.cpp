#include <limits>
#include <vector>
#include <utility>

#include "input_reader.h"

using std::string;
using namespace std::string_literals;
using std::move;

InputReader::InputReader(TransportCatalogue& map, std::istream& input)
: map_(map)
, input_(input){
    CreateQueue();
    ParseQueueStops();
    ParseQueueBuses();
    ParseQueueLengths();
}

void InputReader::CreateQueue(){
    int num_requests;
    input_ >> num_requests;
    input_.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    for(int i = 0; i < num_requests; ++i){
        string line;
        std::getline(input_, line);
        int first_ch = line.find_first_not_of(' ');
        int last_ch = line.find_first_of(' ', first_ch);
        string key_word = line.substr(first_ch, last_ch - first_ch);
        int next_ch = line.find_first_not_of(' ', last_ch);
        line.erase(0, next_ch);
        (key_word == "Stop"s) ? 
        queue_stops.push_back(move(line)) : queue_buses.push_back(move(line));
    }
}

void InputReader::ParseQueueStops(){
    for(string& line : queue_stops){
        int separator = line.find(':');
        string name_stop = line.substr(0, separator);
        name_stop = name_stop.substr(0, name_stop.find_last_not_of(' ') + 1);
        int start_num = line.find_first_not_of(' ', separator + 1);
        separator = line.find(',', separator);
        string latitude = line.substr(start_num, separator - start_num);
        latitude.erase(latitude.begin() + latitude.find_last_not_of(' ') + 1, latitude.end());
        start_num = line.find_first_not_of(' ', separator + 1);
        separator = line.find(',', start_num);
        string longitude = line.substr(start_num, separator - start_num);
        longitude.erase(longitude.begin() + longitude.find_last_not_of(' ') + 1, longitude.end());
        if(separator != -1){
            queue_lengths[name_stop] = line.substr(separator + 1);
        }
        Stop stop;
        stop.name_stop = move(name_stop);
        stop.latitude = std::stod(move(latitude));
        stop.longitude = std::stod(move(longitude));
        map_.AddStop(move(stop));
    }
}

void InputReader::ParseQueueBuses(){
    for(string& line : queue_buses){
        int separator = line.find(':');
        string name_bus = line.substr(0, separator);
        name_bus = name_bus.substr(0, name_bus.find_last_not_of(' ') + 1);
        std::vector<string> stops;
        char sep = '>';
        int next_separator = line.find(sep, separator + 1);
        bool circlular = true;
        if(next_separator == -1){
            circlular = false;
            sep = '-';
            next_separator = line.find(sep, separator + 1);
        }
        int start_name_stop;
        string stop;
        do{
            start_name_stop = line.find_first_not_of(' ', separator + 1);
            stop = line.substr(start_name_stop, next_separator - start_name_stop);
            stop.erase(stop.begin() + stop.find_last_not_of(' ') + 1, stop.end());
            stops.push_back(move(stop));
            separator = next_separator;
            next_separator = line.find(sep, separator + 1);
        } while(next_separator != -1);
        start_name_stop = line.find_first_not_of(' ', separator + 1);
        stop = line.substr(start_name_stop, line.find_last_not_of(' ') - start_name_stop + 1);
        stops.push_back(move(stop));
        map_.AddBus(move(name_bus), move(stops), circlular);
    }
}

void InputReader::ParseQueueLengths(){
    for(auto& [name_stop, lengths] : queue_lengths){
        std::unordered_map<string, int> length_to_stop;
        int separator;
        do{
            separator = lengths.find(',');
            int first_ch = lengths.find_first_not_of(' ');
            int last_ch = lengths.find('m');
            int length = std::stoi(lengths.substr(first_ch, last_ch - first_ch));
            last_ch = lengths.find('o', last_ch);
            first_ch = lengths.find_first_not_of(' ', last_ch + 1);
            string name = (separator == -1)
            ? lengths.substr(first_ch)
            : lengths.substr(first_ch, separator - first_ch);
            last_ch = name.find_last_not_of(' ');
            name.erase(name.begin() + last_ch + 1, name.end());
            length_to_stop[move(name)] = length;
            lengths.erase(lengths.begin(), lengths.begin() + separator + 1);
        } while(separator != -1);
        map_.SetLengths(move(name_stop), move(length_to_stop));
    }
}