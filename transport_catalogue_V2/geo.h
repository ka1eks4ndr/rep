#pragma once

#include <functional>

namespace geo {

struct Coordinates {

    bool operator==(const Coordinates& other) const;

    double lat; // Широта
    double lng; // Долгота
};

class CoordinatesHasher {
private:
    std::hash<double>d_hasher_;
       
public:
    size_t operator()(const Coordinates& coordinates) const;
};


double ComputeDistance(Coordinates from, Coordinates to);

}  // namespace geo