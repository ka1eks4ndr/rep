#include "map_renderer.h"

namespace renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

//---------------SphereProjector----------------
svg::Point  SphereProjector::operator()(geo::Coordinates coords) const {
    return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
}


//---------------MapRenderer---------------------
MapRenderer::MapRenderer(RenderSettings& render_settings,SphereProjector& sphere_projector )
                : render_settings_(render_settings),
                sphere_projector_(sphere_projector),
                palette_size(render_settings.color_palette.size()) {


}

const std::vector<svg::Color>* MapRenderer::GetPallete () const {
    return &render_settings_.color_palette;
} 

svg::Polyline MapRenderer::DrawBus (const std::vector<geo::Coordinates>& container_coordinates, int color_index) const {
    svg::Polyline result_bus;
    
    for (const auto& coordinates : container_coordinates) {
        result_bus.AddPoint(sphere_projector_(coordinates));
    }
    result_bus.SetStrokeWidth(render_settings_.line_width).
                SetStrokeLineCap(svg::StrokeLineCap::ROUND).
                SetStrokeLineJoin(svg::StrokeLineJoin::ROUND).
                SetStrokeColor(render_settings_.color_palette[color_index]).
                SetFillColor("none");
                

    return result_bus;
}

svg::Text MapRenderer::DrawNameBus (const geo::Coordinates& coordinates, const std::string& name, int fill_color_index) const {
    svg::Text result_text;
    svg::Point point=sphere_projector_(coordinates);
    result_text.SetPosition(point).
                SetOffset(svg::Point{render_settings_.bus_label_offset_dx,
                                    render_settings_.bus_label_offset_dy}).
                SetFontSize(render_settings_.bus_label_font_size).
                SetFontFamily("Verdana").
                SetFontWeight("bold").
                SetData(name).
                SetFillColor(render_settings_.color_palette[fill_color_index]);
    return result_text;
}

svg::Text MapRenderer::DrawLabelBus (const geo::Coordinates& coordinates, const std::string& name) const {
    svg::Text result_text;
    svg::Point point=sphere_projector_(coordinates);
    result_text.SetPosition(point).
                SetOffset(svg::Point{render_settings_.bus_label_offset_dx,
                                    render_settings_.bus_label_offset_dy}).
                SetFontSize(render_settings_.bus_label_font_size).
                SetFontFamily("Verdana").
                SetFontWeight("bold").
                SetData(name).
                SetFillColor(render_settings_.underlayer_color).
                SetStrokeColor(render_settings_.underlayer_color).
                SetStrokeWidth(render_settings_.underlayer_width).
                SetStrokeLineCap(svg::StrokeLineCap::ROUND).
                SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    return result_text;
}

svg::Circle MapRenderer::DrawStops (const geo::Coordinates& coordinates) const {
    svg::Circle result_stop;
    svg::Point point=sphere_projector_(coordinates);
    result_stop.SetCenter(point).
                SetRadius(render_settings_.stop_radius).
                SetFillColor("white");
    return result_stop;
}

svg::Text MapRenderer::DrawLabelStop (const geo::Coordinates& coordinates, const std::string& name) const {
    svg::Text result_text;
    svg::Point point=sphere_projector_(coordinates);
    result_text.SetPosition(point).
    SetOffset(svg::Point{render_settings_.stop_label_offset_dx,
            render_settings_.stop_label_offset_dy}).
    SetFontSize(render_settings_.stop_label_font_size).
    SetFontFamily("Verdana").
    SetData(name).
    SetFillColor(render_settings_.underlayer_color).
    SetStrokeColor(render_settings_.underlayer_color).
    SetStrokeWidth(render_settings_.underlayer_width).
    SetStrokeLineCap(svg::StrokeLineCap::ROUND).
    SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    return result_text;
}

svg::Text MapRenderer::DrawNameStop (const geo::Coordinates& coordinates, const std::string& name) const{
    svg::Text result_text;
    svg::Point point=sphere_projector_(coordinates);
    result_text.SetPosition(point).
    SetOffset(svg::Point{render_settings_.stop_label_offset_dx,
            render_settings_.stop_label_offset_dy}).
    SetFontSize(render_settings_.stop_label_font_size).
    SetFontFamily("Verdana").
    SetData(name).
    SetFillColor("black");
    return result_text;
}

const renderer::RenderSettings MapRenderer::GetRendererSettings () const {
    return render_settings_;
}

} //namespace renderer 