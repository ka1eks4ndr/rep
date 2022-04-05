#include "svg.h"

namespace svg {

using namespace std::literals;

uint8_t Lerp(uint8_t from, uint8_t to, double t) {
    return static_cast<uint8_t>(std::round((to - from) * t + from));
}

Rgb Lerp(svg::Rgb from, svg::Rgb to, double t) {
    return {Lerp(from.red, to.red, t), Lerp(from.green, to.green, t), Lerp(from.blue, to.blue, t)};
}

// ---------- OstreamColorPrinter ------------------

    void OstreamColorPrinter::operator()(std::monostate) const {
        out << "none";
    }
    void OstreamColorPrinter::operator()(std::string str) const {
        out << str;
    }
    void OstreamColorPrinter::operator()(svg::Rgb rgb) const {
        out << "rgb("s << std::to_string(rgb.red) << ","s << std::to_string(rgb.green) << ","s << std::to_string(rgb.blue) << ")"s;
    }
    void OstreamColorPrinter::operator()(svg::Rgba rgba) const {
                out << "rgba("s + std::to_string(rgba.red) << ","s << std::to_string(rgba.green) << ","s << std::to_string(rgba.blue) <<
         ","s << rgba.opacity << ")"s;
    }


std::ostream& operator<<(std::ostream& os, const StrokeLineCap& obj) {
    switch (static_cast<int>(obj))
    {
    case 0 :
        os<<"butt";
        break;
    case 1 :
        os<<"round";
        break;
    case 2 :
        os<<"square";
        break;
    
    default:
        break;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const StrokeLineJoin& obj) {
    switch (static_cast<int>(obj))
    {
    case 0 :
        os << "arcs";
        break;
    case 1 :
        os << "bevel";
        break;
    case 2 :
        os << "miter";
        break;
    case 3 :
        os << "miter-clip";
        break;
    case 4 :
        os << "round";
        break;
    default:
        break;
    }
    return os;
}

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
    out << "r=\""sv << radius_ << "\" "sv;
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(Point point){
    polyline_.push_back(point);
    return *this;

}

void Polyline::RenderObject(const RenderContext& context) const  {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool first = true;
    for (const auto& point : polyline_) {
        if (first) {
            out << point.x << ',' << point.y;
            first = false;
        } else {
            out <<' ' << point.x << ',' << point.y;
        }
    }
    out << "\"";
    RenderAttrs(context.out);
    out << "/>"sv;
}

// ---------- Text ------------------

Text& Text::SetPosition(Point pos) {
    position_ = pos;
    return *this;
}

// Задаёт смещение относительно опорной точки (атрибуты dx, dy)
Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

// Задаёт размеры шрифта (атрибут font-size)
Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

// Задаёт название шрифта (атрибут font-family)
Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = font_family;
    return *this;
}

// Задаёт толщину шрифта (атрибут font-weight)
Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = font_weight;
    return *this;
}

// Задаёт текстовое содержимое объекта (отображается внутри тега text)
Text& Text::SetData(std::string data) {
    data_ = data;
    return *this;
    
}

void Text::RenderObject(const RenderContext& context) const  {
   
    auto& out = context.out;
    out << "<text"; 
    RenderAttrs(context.out);
    out<<" x=\""sv << position_.x << "\" y=\""sv << position_.y << "\" "sv;
    out << "dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\" "sv;
    out << "font-size=\""sv << size_ << "\" "sv;
    if (!font_family_.empty()) {
        out << "font-family=\""sv << font_family_ << "\" "sv;
    }
    if (!font_weight_.empty()) {
        out << "font-weight=\""sv << font_weight_ << "\""sv;
    }
    out << ">"sv;
    std::string svg_text;
    for (const auto& c : data_ ) {
        switch (c)
        {
        case '"':
            svg_text += "&quot;";
            break;
        case '\'':
            svg_text += "&apos;";
            break;
        case '<':
            svg_text += "&lt;";
            break;
        case '>':
            svg_text += "&gt;";
            break;
        case '&':
            svg_text += "&amp;";
            break;
        default:
            svg_text.push_back(c);
        }
    }
    out << svg_text;
    out << "</text>"sv;

}

// ---------- Document ------------------ 

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back (std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
    RenderContext ctx(out, 2, 2);
    for (const auto& obj : objects_) {
        obj->Render(ctx);
    }
    out << "</svg>";
}


}  // namespace svg




