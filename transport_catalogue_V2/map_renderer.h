#pragma once

#include "domain.h"
#include "svg.h"
#include "geo.h"

#include <cmath>
#include <algorithm>

namespace renderer {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
                    double max_height, double padding)
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                  return lhs.lng < rhs.lng;
              });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                  return lhs.lat < rhs.lat;
              });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const; 

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer(RenderSettings& render_settings,SphereProjector& sphere_projector);

    const std::vector<svg::Color>* GetPallete () const; 
    svg::Polyline DrawBus (const std::vector<geo::Coordinates>& container_coordinates, int color_index) const;
    svg::Text DrawNameBus (const geo::Coordinates& coordinates, const std::string& name, int fill_color_index) const;
    svg::Text DrawLabelBus (const geo::Coordinates& coordinates, const std::string& name) const;
    svg::Circle DrawStops (const geo::Coordinates& coordinates) const;
    svg::Text DrawLabelStop (const geo::Coordinates& coordinates, const std::string& name) const;
    svg::Text DrawNameStop (const geo::Coordinates& coordinates, const std::string& name) const;
    const renderer::RenderSettings GetRendererSettings () const;
    
private:
RenderSettings render_settings_;
SphereProjector sphere_projector_;
int palette_size = 0;
int current_color = 0;
};

} //namespace renderer

