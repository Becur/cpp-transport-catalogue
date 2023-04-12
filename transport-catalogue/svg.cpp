#include "svg.h"

namespace svg {

using namespace std::literals;

std::ostream& operator<<(std::ostream& out, const Color& color){
    std::visit(ColorPrinter{out}, color);
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap){
    switch(line_cap){
        case StrokeLineCap::BUTT : out << "butt"sv; break;
        case StrokeLineCap::ROUND : out << "round"sv; break;
        case StrokeLineCap::SQUARE : out << "square"sv; break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join){
    switch(line_join){
        case StrokeLineJoin::ARCS : out << "arcs"sv; break;
        case StrokeLineJoin::BEVEL : out << "bevel"sv; break;
        case StrokeLineJoin::MITER : out << "miter"sv; break;
        case StrokeLineJoin::MITER_CLIP : out << "miter-clip"sv; break;
        case StrokeLineJoin::ROUND : out << "round"sv; break;
    }
    return out;
}

// ---------- Struct ------------------------

Rgb::Rgb(const uint8_t red_c, const uint8_t green_c,const uint8_t blue_c)
: red(red_c), green(green_c), blue(blue_c){}

Rgba::Rgba(const uint8_t red_c, const uint8_t green_c, const uint8_t blue_c, const double opacity_c)
: red(red_c), green(green_c), blue(blue_c), opacity(opacity_c){}

// ---------- ColorPrinter ------------------

void ColorPrinter::operator()(std::monostate){
    out << NoneColor;
}

void ColorPrinter::operator()(const std::string& color){
    out << color;
}

void ColorPrinter::operator()(const Rgb& color){
    out << "rgb("s << static_cast<uint16_t>(color.red) << ","s
    << static_cast<uint16_t>(color.green) << ","s 
    << static_cast<uint16_t>(color.blue) << ")"s;
}

void ColorPrinter::operator()(const Rgba& color){
    out << "rgba("s << static_cast<uint16_t>(color.red) << ","s
    << static_cast<uint16_t>(color.green) << ","s 
    << static_cast<uint16_t>(color.blue) << ","s << color.opacity << ")"s;
}

// ---------- Object ------------------

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();

    // Делегируем вывод тега своим подклассам
    RenderObject(context);

    context.out << std::endl;
}


// ---------- Circle ------------------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}
// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point){
    points.push_back(std::move(point));
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const{
    auto& out = context.out;
    out << "<polyline points=\""sv;
    if(!points.empty()){
        out << points[0].x << ","sv << points[0].y;
    }
    for(size_t i = 1; i < points.size(); ++i){
        out << " "sv << points[i].x << ","sv << points[i].y;
    }
    out << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}
// ------------ Text --------------------

Text& Text::SetPosition(Point pos){
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset){
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size){
    font_size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family){
    font_family_ = std::move(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight){
    font_weight_ = std::move(font_weight);
    return *this;
}

Text& Text::SetData(std::string data){
    data_ = std::move(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const{
    auto& out = context.out;
    out << "<text"sv;
    RenderAttrs(out);
    out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y;
    out << "\" dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y;
    out << "\" font-size=\""sv << font_size_ << "\""sv;
    if(font_family_){
        out << " font-family=\""sv << *font_family_ << "\""sv;
    }
    if(font_weight_){
        out << " font-weight=\""sv << *font_weight_ << "\""sv; 
    }
    out << ">"sv;
    RenderData(out);
    out << "</text>"sv;
}

void Text::RenderData(std::ostream& out) const{
    std::string current_ch;
    for(size_t i = 0; i < data_.size(); ++i){
        current_ch = data_[i];
        if(current_ch == "\""s){
            current_ch = "&quot;"s;
        }
        else if(current_ch == "&"s){
            current_ch = "&amp;"s;
        }
        else if(current_ch == "'"s){
            current_ch = "&apos;"s;
        }
        else if(current_ch == "<"s){
            current_ch = "&lt;"s;
        }
        else if(current_ch == ">"s){
            current_ch = "&gt;"s;
        }
        out << std::move(current_ch);
    }
}
// ---------- Document ------------------


void Document::AddPtr(std::unique_ptr<Object>&& obj){
    if(obj){
        objects.push_back(std::move(obj));
    }
}

void Document::Render(std::ostream& out) const{
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"sv;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n"sv;
    for(const auto& obj : objects){
        out << "  "sv;
        obj->Render(out);
    }
    out << "</svg>"sv;
}

bool Document::Empty() const{
    return objects.empty();
}

}  // namespace svg