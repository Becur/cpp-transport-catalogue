#include "svg.h"
#include "json.h"
#include "transport_catalogue.h"

#include <transport_catalogue.pb.h>

#include <string>
#include <vector>
#include <ostream>

class MapRenderer{
public:
    MapRenderer(const TransportCatalogue& map, const render_settings::Settings& settings);

    void PrintMap(std::ostream& output);
    
private:
    const TransportCatalogue& map_;
    const render_settings::Settings& settings_;
    svg::Document doc_;
    SphereProjector projector;
    std::vector<const Bus*> buses_;
    std::vector<svg::Color> color_palette;
    std::vector<const Stop*> stops_;

    void SetSettings();

    void PreparationBuses();

    void ParseColorPalette();

    svg::Color ParseColor(const render_settings::Color& color) const;

    void DrawLineBuses();

    void DrawNameBuses();

    void CreateNameBus(svg::Point point, std::string text, svg::Color color);

    void PreparationStops();

    void DrawCircleStops();

    void CreateCircleStop(svg::Point point);

    void DrawNameStops();

    void CreateNameStop(svg::Point point, std::string text);
};