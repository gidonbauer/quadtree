#ifndef QT_GEOMETRY_HPP_
#define QT_GEOMETRY_HPP_

#include <concepts>
#include <type_traits>

template <std::floating_point Float = double>
struct Point {
  Float x;
  Float y;

  constexpr auto operator==(const Point& other) const noexcept -> bool {
    return x == other.x && y == other.y;
  }
};

template <std::floating_point Float = double>
struct Box {
  Float x;
  Float y;
  Float w;
  Float h;

  [[nodiscard]] constexpr auto contains_point(const Point<Float>& p) const noexcept -> bool {
    return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
  }

  [[nodiscard]] constexpr auto intersects(const Box<Float>& b) const noexcept -> bool {
    return !((x > (b.x + b.w)) || ((x + w) < b.x) || (y > (b.y + b.h)) || ((y + h) < b.y));
  }
};

#endif  // QT_GEOMETRY_HPP_
