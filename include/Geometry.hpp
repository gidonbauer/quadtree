#ifndef QT_GEOMETRY_HPP_
#define QT_GEOMETRY_HPP_

#include <concepts>
#include <type_traits>

namespace qtree {

template <std::floating_point Float = double>
struct Point {
  Float x;
  Float y;

  constexpr auto operator==(const Point& other) const noexcept -> bool {
    return x == other.x && y == other.y;
  }
};

template <std::floating_point Float = double>
struct Circle {
  Float x;
  Float y;
  Float r;

  [[nodiscard]] constexpr auto contains(const Point<Float>& p) const noexcept -> bool {
    const auto d_sqr = (x - p.x) * (x - p.x) + (y - p.y) * (y - p.y);
    return d_sqr <= r * r;
  }
};

template <std::floating_point Float = double>
struct Box {
  Float x;
  Float y;
  Float w;
  Float h;

  [[nodiscard]] constexpr auto contains(const Point<Float>& p) const noexcept -> bool {
    return p.x >= x && p.x <= x + w && p.y >= y && p.y <= y + h;
  }

  [[nodiscard]] constexpr auto intersects(const Box<Float>& b) const noexcept -> bool {
    return !((x > (b.x + b.w)) || ((x + w) < b.x) || (y > (b.y + b.h)) || ((y + h) < b.y));
  }

  [[nodiscard]] constexpr auto intersects(const Circle<Float>& c) const noexcept -> bool {
    const auto closest_x = (c.x < x) ? x : ((c.x > (x + w)) ? (x + w) : c.x);
    const auto closest_y = (c.y < y) ? y : ((c.y > (y + h)) ? (y + h) : c.y);

    const auto dx = closest_x - c.x;
    const auto dy = closest_y - c.y;

    return (dx * dx + dy * dy) <= c.r * c.r;
  }
};

}  // namespace qtree

#endif  // QT_GEOMETRY_HPP_
