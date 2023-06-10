#include "map_renderer.h"
#include "domain.h"
#include "geo.h"

#include <utility>
#include <algorithm>

using namespace std::literals;
using json::Node;
using std::move;
using svg::Color;

MapRenderer::MapRenderer(const TransportCatalogue& map, const render_settings::Settings& settings)
:map_(map)
, settings_(settings){}

void MapRenderer::PrintMap(std::ostream& output){
    if(doc_.Empty()){
        SetSettings();
        PreparationBuses();
        DrawLineBuses();
        DrawNameBuses();
        PreparationStops();
        DrawCircleStops();
        DrawNameStops();
    }
    doc_.Render(output);
}

void MapRenderer::SetSettings(){
    std::vector <geo::Coordinates> coordinates;
    coordinates.reserve(map_.GetStops().size());
    for(const auto& stop : map_.GetStops()){
        if(!stop.buses.empty()){
            coordinates.push_back({stop.latitude, stop.longitude});
        }
    }
    projector = SphereProjector(coordinates.begin(), coordinates.end(),
    settings_.width(), settings_.height(),
    settings_.padding());

}

void MapRenderer::PreparationBuses(){
    buses_.reserve(map_.GetBuses().size());
    for(const Bus& bus : map_.GetBuses()){
        buses_.push_back(&bus);
    }
    std::sort(buses_.begin(), buses_.end(), 
    [](const Bus* lbus, const Bus* rbus){
        return lbus->name_bus < rbus->name_bus;
    });
}

void MapRenderer::DrawLineBuses(){
    ParseColorPalette();
    int i = 0;
    for(const auto* bus : buses_){
        if(bus->stops.empty()){
            continue;
        }
        svg::Polyline polyline;
        polyline
        .SetStrokeWidth(settings_.line_width())
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
        .SetFillColor(svg::NoneColor)
        .SetStrokeColor(color_palette[i++ % color_palette.size()]);
        for(const auto* stop : bus->stops){
            polyline.AddPoint(projector({stop->latitude, stop->longitude}));
        }
        if(!bus->circular){
            bool is_first = true;
            std::for_each(bus->stops.rbegin(), bus->stops.rend(),
            [&](const auto* stop){
                if(is_first){
                    is_first = false;
                }
                else{
                    polyline.AddPoint(projector({stop->latitude, stop->longitude}));
                }
            });
        }
        doc_.Add(move(polyline));
    }
}

void MapRenderer::ParseColorPalette(){
    color_palette.reserve(settings_.color_palette_size());
    for(const auto& color : settings_.color_palette()){
        color_palette.push_back(ParseColor(color));
    }
}

Color MapRenderer::ParseColor(const render_settings::Color& color) const{
    if(color.has_color()){
        return Color(color.color().word());
    }
    else{
        if(color.has_rgb()){
            return Color(svg::Rgb(color.rgb().red(),
            color.rgb().green(), color.rgb().blue()));
        }
        else{
            return Color(svg::Rgba(color.rgba().rgb().red(),
            color.rgba().rgb().green(), color.rgba().rgb().blue(), color.rgba().alpha()));
        }
    }
}

void MapRenderer::DrawNameBuses(){
    int i = 0;
    for(const auto* bus : buses_){
        CreateNameBus(projector({bus->stops[0]->latitude, 
        bus->stops[0]->longitude}), bus->name_bus, 
        color_palette[i % color_palette.size()]);
        if((!bus->circular) && (bus->stops[0] != bus->stops.back())){
            CreateNameBus(projector({bus->stops.back()->latitude, 
            bus->stops.back()->longitude}), bus->name_bus, 
            color_palette[i % color_palette.size()]);
        }
        ++i;
    }
}

void MapRenderer::CreateNameBus(svg::Point point, std::string name_bus, Color color){
    svg::Text name;
    name
    .SetPosition(point)
    .SetOffset({settings_.bus_label_offset(0), settings_.bus_label_offset(1)})
    .SetFontSize(settings_.bus_label_font_size())
    .SetFontFamily("Verdana"s)
    .SetFontWeight("bold"s)
    .SetData(name_bus);

    svg::Text base;
    base = name;

    name
    .SetFillColor(color);

    Color under_color = ParseColor(settings_.underlayer_color());
    base
    .SetFillColor(under_color)
    .SetStrokeColor(move(under_color))
    .SetStrokeWidth(settings_.underlayer_width())
    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc_.Add(move(base));
    doc_.Add(move(name));
}

void MapRenderer::PreparationStops(){
    stops_.reserve(map_.GetStops().size());
    for(const Stop& stop : map_.GetStops()){
        stops_.push_back(&stop);
    }
    std::sort(stops_.begin(), stops_.end(), 
    [](const Stop* lbus, const Stop* rbus){
        return lbus->name_stop < rbus->name_stop;
    });
}


void MapRenderer::DrawCircleStops(){
    for(const Stop* stop : stops_){
        if(!stop->buses.empty()){
            CreateCircleStop(projector({stop->latitude, stop->longitude}));
        }
    }
}

void MapRenderer::CreateCircleStop(svg::Point point){
    svg::Circle circle;
    circle
    .SetCenter(point)
    .SetFillColor(Color("white"s))
    .SetRadius(settings_.stop_radius());
    doc_.Add(move(circle));
}

void MapRenderer::DrawNameStops(){
    for(const auto* stop : stops_){
        if(!stop->buses.empty()){
            CreateNameStop(projector({stop->latitude, stop->longitude}), stop->name_stop);
        }
    }
}

void MapRenderer::CreateNameStop(svg::Point point, std::string name_stop){
    svg::Text name;
    name
    .SetPosition(point)
    .SetOffset({settings_.stop_label_offset(0),
    settings_.stop_label_offset(1)})
    .SetFontSize(settings_.stop_label_font_size())
    .SetFontFamily("Verdana"s)
    .SetData(name_stop);

    svg::Text base;
    base = name;

    name
    .SetFillColor(Color("black"s));

    Color under_color = ParseColor(settings_.underlayer_color());
    base
    .SetFillColor(under_color)
    .SetStrokeColor(move(under_color))
    .SetStrokeWidth(settings_.underlayer_width())
    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    doc_.Add(move(base));
    doc_.Add(move(name));
}