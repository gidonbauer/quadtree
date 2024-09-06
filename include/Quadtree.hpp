#ifndef QT_QUADTREE_HPP_
#define QT_QUADTREE_HPP_

#include <cassert>
#include <concepts>
#include <format>
#include <iostream>
#include <type_traits>
#include <variant>
#include <vector>

#include "Geometry.hpp"
#include "Macros.hpp"

namespace qtree {

namespace detail {
template <std::floating_point Float, size_t MAX_ENTRIES = 10UL>
class QuadtreeNode {
  //     x_split
  //        |
  //        V
  //   +----+----+
  //   | 10 | 11 |
  // y +----+----+ <- y_split
  //   | 00 | 01 |
  //   +----+----+
  //        x

  enum : size_t {
    BOT_LEFT,   // 00
    BOT_RIGHT,  // 01
    TOP_LEFT,   // 10
    TOP_RIGHT,  // 11
    NUM_SUBNODES,
  };

  enum : size_t { SUBNODES, INDICES };

  std::variant<std::vector<QuadtreeNode>, std::vector<size_t>> m_content = std::vector<size_t>{};
  Box<Float> m_extend{};

 public:
  constexpr QuadtreeNode(Box<Float> extend) noexcept
      : m_extend(std::move(extend)) {}

  [[nodiscard]]
  constexpr auto is_leaf() const noexcept -> bool {
    return std::holds_alternative<std::vector<size_t>>(m_content);
  }

  constexpr auto insert(const Point<Float>& pos,
                        size_t idx,
                        const std::vector<Point<Float>>& all_pos) noexcept -> bool {
    assert(m_extend.contains(pos));

    if (is_leaf()) {
      const auto idx_vec_size = std::get<INDICES>(m_content).size();
      if (idx_vec_size < MAX_ENTRIES) {
        std::get<INDICES>(m_content).push_back(idx);
        return true;
      } else {
        const auto idx_vec = std::get<INDICES>(m_content);

        const auto half_w = m_extend.w / static_cast<Float>(2);
        const auto half_h = m_extend.h / static_cast<Float>(2);

        const auto x_split = m_extend.x + half_w;
        const auto y_split = m_extend.y + half_h;

        m_content      = std::vector<QuadtreeNode<Float, MAX_ENTRIES>>{};
        auto& subnodes = std::get<SUBNODES>(m_content);
        subnodes.reserve(NUM_SUBNODES);

        // BOT_LEFT
        subnodes.emplace_back(Box<Float>{
            .x = m_extend.x,
            .y = m_extend.y,
            .w = half_w,
            .h = half_h,
        });
        // BOT_RIGHT
        subnodes.emplace_back(Box<Float>{
            .x = x_split,
            .y = m_extend.y,
            .w = half_w,
            .h = half_h,
        });
        // TOP_LEFT
        subnodes.emplace_back(Box<Float>{
            .x = m_extend.x,
            .y = y_split,
            .w = half_w,
            .h = half_h,
        });
        // TOP_RIGHT
        subnodes.emplace_back(Box<Float>{
            .x = x_split,
            .y = y_split,
            .w = half_w,
            .h = half_h,
        });

        for (const auto& i : idx_vec) {
          insert(all_pos[i], i, all_pos);
        }
        return insert(pos, idx, all_pos);
      }
    } else {
      const auto x_split = m_extend.x + m_extend.w / static_cast<Float>(2);
      const auto y_split = m_extend.y + m_extend.h / static_cast<Float>(2);

      const size_t grid_pos = 2UL * (pos.y > y_split) + 1UL * (pos.x > x_split);
      assert(grid_pos < NUM_SUBNODES);

      return std::get<SUBNODES>(m_content)[grid_pos].insert(pos, idx, all_pos);
    }
  }

  [[nodiscard]] constexpr auto find(const Point<Float>& pos) const noexcept -> std::vector<size_t> {
    assert(m_extend.contains(pos));

    if (is_leaf()) {
      return std::get<INDICES>(m_content);
    } else {
      const auto x_split = m_extend.x + m_extend.w / static_cast<Float>(2);
      const auto y_split = m_extend.y + m_extend.h / static_cast<Float>(2);

      const size_t grid_pos = 2UL * (pos.y > y_split) + 1UL * (pos.x > x_split);
      assert(grid_pos < NUM_SUBNODES);

      return std::get<SUBNODES>(m_content)[grid_pos].find(pos);
    }
  }

  template <template <typename> class Shape>
  [[nodiscard]] constexpr auto
  find(const Shape<Float>& shape) const noexcept -> std::vector<size_t> {
    assert(m_extend.intersects(shape));

    if (is_leaf()) {
      return std::get<INDICES>(m_content);
    } else {
      std::vector<size_t> idxs{};
      for (const auto& subnode : std::get<SUBNODES>(m_content)) {
        if (subnode.m_extend.intersects(shape)) {
          const auto tmp_idxs = subnode.find(shape);
          idxs.insert(std::end(idxs), std::begin(tmp_idxs), std::end(tmp_idxs));
        }
      }
      return idxs;
    }
  }

  constexpr void print(size_t indent = 0UL) const noexcept {
    if (is_leaf()) {
      std::cout << std::string(indent, ' ') << '[';
      for (auto idx : std::get<INDICES>(m_content)) {
        std::cout << idx << ", ";
      }
      std::cout << "]\n";
    } else {
      for (const auto& subnode : std::get<SUBNODES>(m_content)) {
        subnode.print(indent + 2UL);
        ;
      }
    }
  }
};

}  // namespace detail

template <typename Data, std::floating_point Float = double, size_t MAX_ENTRIES = 10UL>
class Quadtree {
  std::vector<Point<Float>> m_pos{};
  std::vector<Data> m_data{};

  Box<Float> m_bounding_box{};

  detail::QuadtreeNode<Float, MAX_ENTRIES> m_root;

 public:
  constexpr Quadtree(Box<Float> bounding_box)
      : m_bounding_box(std::move(bounding_box)),
        m_root(detail::QuadtreeNode<Float, MAX_ENTRIES>(m_bounding_box)) {}

  [[nodiscard]] constexpr auto pos() const noexcept -> const std::vector<Point<Float>>& {
    return m_pos;
  }
  [[nodiscard]] constexpr auto data() const noexcept -> const std::vector<Data>& { return m_data; }

  constexpr auto insert(Point<Float> pos, Data data) noexcept -> bool {
    if (!m_bounding_box.contains(pos)) {
      return false;
    }

    // TODO: Assumes the pos is not in quadtree
    m_pos.push_back(std::move(pos));
    m_data.push_back(std::move(data));
    assert(m_pos.size() == m_data.size() && m_pos.size() >= 1UL);
    const auto idx = m_pos.size() - 1UL;

    m_root.insert(m_pos[idx], idx, m_pos);

    return true;
  }

  [[nodiscard]] constexpr auto find(const Point<Float>& pos) -> Data& {
    if (!m_bounding_box.contains(pos)) {
      throw std::runtime_error(
          std::format("{}: Position {{{}, {}}} is not in bounding_box {{[{}, {}], [{}, {}]}}.",
                      QT_ERROR_LOC(),
                      pos.x,
                      pos.y,
                      m_bounding_box.x,
                      m_bounding_box.x + m_bounding_box.w,
                      m_bounding_box.y,
                      m_bounding_box.y + m_bounding_box.h));
    }

    const auto possible_idxs = m_root.find(pos);
    for (size_t idx : possible_idxs) {
      if (pos == m_pos[idx]) {
        return m_data[idx];
      }
    }
    throw std::runtime_error(
        std::format("{}: Position {{{}, {}}} is not in quadtree.", QT_ERROR_LOC(), pos.x, pos.y));
  }

  template <template <typename> class Shape>
  [[nodiscard]] constexpr auto find(const Shape<Float>& shape) const -> std::vector<Data> {
    using namespace std::string_literals;
    if (!m_bounding_box.intersects(shape)) {
      throw std::runtime_error(QT_ERROR_LOC() + ": Search shape does not intersect bounding_box."s);
    }

    const auto possible_idxs = m_root.find(shape);

    std::vector<Data> res{};
    res.reserve(possible_idxs.size());
    for (size_t idx : possible_idxs) {
      if (shape.contains(m_pos[idx])) {
        res.push_back(m_data[idx]);
      }
    }

    return res;
  }

  constexpr void print() const noexcept {
    for (size_t i = 0; i < m_pos.size(); ++i) {
      std::cout << "{" << m_pos[i].x << ", " << m_pos[i].y << "} -> " << m_data[i] << '\n';
    }
  }

  constexpr void print_root() const noexcept { m_root.print(); }
};

}  // namespace qtree

#endif  // QT_QUADTREE_HPP_
