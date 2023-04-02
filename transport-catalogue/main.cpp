#include "input_reader.h"
#include "transport_catalogue.h"
#include "stat_reader.h"

int main(){
    TransportCatalogue map;
    InputReader input(map);
    StatReader stat(map);
}