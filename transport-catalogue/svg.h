#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>

namespace svg {

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

/*
 * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
 * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
 */
struct RenderContext {
    RenderContext(std::ostream& out)
        : out(out) {
    }

    RenderContext(std::ostream& out, int indent_step, int indent = 0)
        : out(out)
        , indent_step(indent_step)
        , indent(indent) {
    }

    RenderContext Indented() const {
        return {out, indent_step, indent + indent_step};
    }

    void RenderIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

/*
 * Абстрактный базовый класс Object служит для унифицированного хранения
 * конкретных тегов SVG-документа
 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
 */
class Object {
public:
    void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer{
public:
    template <typename T>
    void Add(const T& obj);

    virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;
protected:
    ~ObjectContainer() = default;
};

class Drawable{
public:
    virtual void Draw(ObjectContainer& container) const = 0;

    virtual ~Drawable() = default;
};

struct Rgb {
    Rgb() = default;
    Rgb(const uint8_t red_c, const uint8_t green_c,const uint8_t blue_c);

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
};

struct Rgba{
    Rgba() = default;
    Rgba(const uint8_t red_c, const uint8_t green_c, const uint8_t blue_c, const double opacity_c);

    uint8_t red = 0;
    uint8_t green = 0;
    uint8_t blue = 0;
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, Rgb, Rgba, std::string>;

inline const Color NoneColor{"none"};

struct ColorPrinter{
    std::ostream& out;

    void operator()(std::monostate);
    void operator()(const std::string& color);
    void operator()(const Rgb& color);
    void operator()(const Rgba& color);
};

std::ostream& operator<<(std::ostream& out, const Color& color);

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& line_cap);

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& line_join);


template<typename Owner>
class PathProps{
public:
    Owner& SetFillColor(Color color);
    Owner& SetStrokeColor(Color color);
    Owner& SetStrokeWidth(double width);
    Owner& SetStrokeLineCap(StrokeLineCap line_cap);
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join);

protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const;

private:

    Owner& AsOwner();

    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap;
    std::optional<StrokeLineJoin> stroke_linejoin;
};

/*
 * Класс Circle моделирует элемент <circle> для отображения круга
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
 */
class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(Point center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

    Point center_;
    double radius_ = 1.0;
};

/*
 * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
 */
class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline() = default;

    // Добавляет очередную вершину к ломаной линии
    Polyline& AddPoint(Point point);

private:
    void RenderObject(const RenderContext& context) const override;

    std::vector<Point> points;
};

/*
 * Класс Text моделирует элемент <text> для отображения текста
 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
 */
class Text final : public Object, public PathProps<Text> {
public:
    Text() = default;

    // Задаёт координаты опорной точки (атрибуты x и y)
    Text& SetPosition(Point pos);

    // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
    Text& SetOffset(Point offset);

    // Задаёт размеры шрифта (атрибут font-size)
    Text& SetFontSize(uint32_t size);

    // Задаёт название шрифта (атрибут font-family)
    Text& SetFontFamily(std::string font_family);

    // Задаёт толщину шрифта (атрибут font-weight)
    Text& SetFontWeight(std::string font_weight);

    // Задаёт текстовое содержимое объекта (отображается внутри тега text)
    Text& SetData(std::string data);

    // Прочие данные и методы, необходимые для реализации элемента <text>

private:
    void RenderObject(const RenderContext& context) const override;

    Point pos_ = {0, 0};
    Point offset_ = {0, 0};
    uint32_t font_size_ = 1;
    std::optional<std::string> font_family_;
    std::optional<std::string> font_weight_;
    std::string data_ = "";

    void RenderData(std::ostream& out) const;
};

class Document : public ObjectContainer {
public:
    // Добавляет в svg-документ объект-наследник svg::Object
    void AddPtr(std::unique_ptr<Object>&& obj) override;

    // Выводит в ostream svg-представление документа
    void Render(std::ostream& out) const;

    bool Empty() const;

private:
    std::vector<std::unique_ptr<Object>> objects;
};

//---------------------------------------------------------------

template<typename T>
void ObjectContainer::Add(const T& obj){
    AddPtr(std::make_unique<T>(obj));
}

template<typename Owner>
Owner& PathProps<Owner>::SetFillColor(Color color) {
    fill_color_ = std::move(color);
    return AsOwner();
}
template<typename Owner>
Owner& PathProps<Owner>::SetStrokeColor(Color color) {
    stroke_color_ = std::move(color);
    return AsOwner();
}
template<typename Owner>
Owner& PathProps<Owner>::SetStrokeWidth(double width){
    stroke_width_ = width;
    return AsOwner();
}
template<typename Owner>
Owner& PathProps<Owner>::SetStrokeLineCap(StrokeLineCap line_cap){
    stroke_linecap = line_cap;
    return AsOwner();
}
template<typename Owner>
Owner& PathProps<Owner>::SetStrokeLineJoin(StrokeLineJoin line_join){
    stroke_linejoin = line_join;
    return AsOwner();
}

template<typename Owner>
void PathProps<Owner>::RenderAttrs(std::ostream& out) const {
    using namespace std::literals;

    if (fill_color_) {
        out << " fill=\""sv << *fill_color_ << "\""sv;
    }
    if (stroke_color_) {
        out << " stroke=\""sv << *stroke_color_ << "\""sv;
    }
    if(stroke_width_){
        out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
    }
    if(stroke_linecap){
        out << " stroke-linecap=\""sv << *stroke_linecap << "\""sv;
    }
    if(stroke_linejoin){
        out << " stroke-linejoin=\""sv << *stroke_linejoin << "\""sv;
    }
}

template<typename Owner>
Owner& PathProps<Owner>::AsOwner() {
    // static_cast безопасно преобразует *this к Owner&,
    // если класс Owner — наследник PathProps
    return static_cast<Owner&>(*this);
}

}  // namespace svg