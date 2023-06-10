#include "serialization.h"

#include <utility>
#include <fstream>

using namespace std::string_literals;

Seriallization::Seriallization(std::istream& input)
: doc(json::Load(input)){
    CreateQueue();
    ParseQueueStops();
    ParseQueueBuses();
    ParseQueueLengths();
    ParseRenderSettings();
    BuildRouter();
    StartParseRouter();
    Save();
}

void Seriallization::CreateQueue(){
    const auto& base_requests = doc.GetRoot().AsDict().at("base_requests"s);
    for(const auto& node : base_requests.AsArray()){
        (node.AsDict().at("type"s).AsString() == "Stop"s)
        ? queue_stops.push_back(&node)
        : queue_buses.push_back(&node);
    }
}

void Seriallization::ParseQueueStops(){
    size_t i = 0;
    for(const json::Node* node : queue_stops){
        catalog.add_stop();
        const auto& stop_js = node->AsDict();
        queue_lengths.push_back({stop_js.at("name"s).AsString(), &stop_js.at("road_distances"s)});

        transport::Stop stop;
        stop.set_name(stop_js.at("name"s).AsString());
        stop.set_latitude(stop_js.at("latitude"s).AsDouble());
        stop.set_longitude(stop_js.at("longitude"s).AsDouble());
        *catalog.mutable_stop(i) = std::move(stop);
        ++i;
    }
}

void Seriallization::ParseQueueBuses(){
    size_t i = 0;
    for(const json::Node* node : queue_buses){
        catalog.add_bus();
        const auto& bus_js = node->AsDict();
        const auto& stops_js = bus_js.at("stops"s).AsArray();
        transport::Bus bus;
        bus.set_name(std::move(bus_js.at("name"s).AsString()));
        for(size_t i = 0; i < stops_js.size(); ++i){
            bus.add_name_stop();
            *bus.mutable_name_stop(i) = std::move(stops_js[i].AsString());
        }
        bus.set_circular(bus_js.at("is_roundtrip"s).AsBool());
        *catalog.mutable_bus(i) = std::move(bus);
        ++i;
    }
}

void Seriallization::ParseQueueLengths(){
    size_t i = 0;
    for(const auto& [name, node] : queue_lengths){
        catalog.add_lenghts();
        transport::Lenghts lenghts;
        lenghts.set_name_stop(std::move(std::string(name)));
        for(const auto& [string, node] : node->AsDict()){
            lenghts.add_other_stop(string);
            lenghts.add_lenght_to_other_stop(node.AsInt());
        }
        *catalog.mutable_lenghts(i) = std::move(lenghts);
        ++i;
    }
}

void Seriallization::ParseRenderSettings(){
    const auto& render_settings = doc.GetRoot().AsDict().at("render_settings"s).AsDict();
    settings.set_width(render_settings.at("width"s).AsDouble()); 
    settings.set_height(render_settings.at("height"s).AsDouble()); 
    settings.set_padding(render_settings.at("padding"s).AsDouble()); 
    settings.set_stop_radius(render_settings.at("stop_radius"s).AsDouble()); 
    settings.set_line_width(render_settings.at("line_width"s).AsDouble()); 
    settings.set_bus_label_font_size(render_settings.at("bus_label_font_size"s).AsInt()); 
    settings.set_stop_label_font_size(render_settings.at("stop_label_font_size"s).AsInt()); 
    settings.set_underlayer_width(render_settings.at("underlayer_width"s).AsDouble());
    for(const json::Node& node : render_settings.at("bus_label_offset"s).AsArray()){
        settings.add_bus_label_offset(node.AsDouble());
    }
    for(const json::Node& node : render_settings.at("stop_label_offset"s).AsArray()){
        settings.add_stop_label_offset(node.AsDouble());
    }
    *settings.mutable_underlayer_color() = ParseColor(render_settings.at("underlayer_color"s));
    size_t i = 0;
    for(const json::Node& node : render_settings.at("color_palette"s).AsArray()){
        settings.add_color_palette();
        *settings.mutable_color_palette(i) = ParseColor(node);
        ++i;
    }
}

render_settings::Color Seriallization::ParseColor(const json::Node& node) const{
    render_settings::Color color;
    if(node.IsString()){
        color.mutable_color()->set_word(node.AsString());
    }
    else{
        render_settings::RGB rgb;
        rgb.set_red(node.AsArray()[0].AsInt());
        rgb.set_green(node.AsArray()[1].AsInt());
        rgb.set_blue(node.AsArray()[2].AsInt());
        if(node.AsArray().size() == 3){
            *color.mutable_rgb() = std::move(rgb);
        }
        else{
            render_settings::RGBA rgba;
            *rgba.mutable_rgb() = std::move(rgb);
            rgba.set_alpha(node.AsArray()[3].AsDouble());
            *color.mutable_rgba() = std::move(rgba);
        }
    }
    return color;
}

void Seriallization::BuildRouter(){
    saved_catalog = Deserialization().GetTransportCatalogue(catalog);
    const auto& routing_settings = doc.GetRoot().AsDict().at("routing_settings"s).AsDict();
    saved_router.emplace(saved_catalog, routing_settings.at("bus_wait_time").AsInt(), routing_settings.at("bus_velocity").AsDouble());
}

void Seriallization::StartParseRouter(){
    ParseSaveGraph();
    ParseSaveRouter();
    ParseSaveTransportRoute();
}

void Seriallization::ParseSaveGraph(){
    auto save_graph = saved_router.value().SaveGraph();
    size_t i = 0;
    for(const auto& edge : save_graph.first){
        graph.add_edge();
        save::Edge new_edge;
        new_edge.set_from(edge.from);
        new_edge.set_to(edge.to);
        new_edge.set_weight(edge.weight);
        new_edge.set_count(edge.count);
        *graph.mutable_edge(i++) = std::move(new_edge);
    }
    i = 0;
    for(const auto& vec : save_graph.second){
        for(const auto& edge_id : vec){
            graph.add_incidence_lists(edge_id);
            ++i;
        }
        graph.add_size_list(std::exchange(i, 0));
    }
}

void Seriallization::ParseSaveRouter(){
    auto save_router = saved_router.value().SaveRouter();
    size_t i = 0;
    size_t last_i = 0;
    for(const auto& vec : save_router){
        for(const auto& data : vec){
            routes_internal_data.add_option();
            if(data.has_value()){
                save::RouteInternalData new_data;
                new_data.set_weight(data.value().weight);
                if(data.value().prev_edge.has_value()){
                    new_data.mutable_prev_edge()->set_edge_id(data.value().prev_edge.value());
                }
                *routes_internal_data.mutable_option(i)->mutable_value() = std::move(new_data);
            }
            i++;
        }
        routes_internal_data.add_sizes_data(i - std::exchange(last_i, i));
    }
}

void Seriallization::ParseSaveTransportRoute(){
    const auto& routing_settings = doc.GetRoot().AsDict().at("routing_settings"s).AsDict();
    saved_router.emplace(saved_catalog, routing_settings.at("bus_wait_time").AsInt(), routing_settings.at("bus_velocity").AsDouble());
    transport_route.set_bus_wait_time(routing_settings.at("bus_wait_time").AsInt());
    transport_route.set_bus_velocity(routing_settings.at("bus_velocity").AsDouble());
    auto save_router = saved_router.value().Save();
    size_t i = 0;
    for(auto& [id, bus_name] : save_router.first){
        transport_route.add_edge_id(id);
        transport_route.add_name_bus();
        *transport_route.mutable_name_bus(i) = std::move(bus_name);
        ++i;
    }
    i = 0;
    for(auto& [id, stop_name] : save_router.second){
        transport_route.add_stop_id(id);
        transport_route.add_name_stop();
        *transport_route.mutable_name_stop(i) = std::move(stop_name);
        ++i;
    }
}

void Seriallization::Save(){
    std::ofstream out(doc.GetRoot().AsDict().at("serialization_settings"s).AsDict().at("file"s).AsString(), std::ios::out | std::ios::binary);
    ObjectProto obj;
    *obj.mutable_catalogue() = std::move(catalog);
    *obj.mutable_settings() = std::move(settings);
    *obj.mutable_graph() = std::move(graph);
    *obj.mutable_routes_internal_data() = std::move(routes_internal_data);
    *obj.mutable_transport_route() = std::move(transport_route);
    obj.SerializeToOstream(&out);
}