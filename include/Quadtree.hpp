#ifndef QT_QUADTREE_HPP_
#define QT_QUADTREE_HPP_

#include <cassert>
#include <concepts>
#include <iostream>
#include <type_traits>
#include <variant>
#include <vector>

#include "Geometry.hpp"
#include "Macros.hpp"

template <std::floating_point Float>
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
  static constexpr size_t MAX_ENTRIES = 10ul;

  bool m_is_leaf                                                         = true;
  std::variant<std::vector<QuadtreeNode>, std::vector<size_t>> m_content = std::vector<size_t>{};
  Box<Float> m_extend{};

 public:
  constexpr QuadtreeNode(Box<Float> extend) noexcept
      : m_extend(std::move(extend)) {}

  constexpr auto insert(const Point<Float>& pos,
                        size_t idx,
                        const std::vector<Point<Float>>& all_pos) noexcept -> bool {
    assert(m_extend.contains_point(pos));

    if (m_is_leaf) {
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

        m_is_leaf      = false;
        m_content      = std::vector<QuadtreeNode<Float>>{};
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

      const size_t grid_pos = 2ul * (pos.y > y_split) + 1ul * (pos.x > x_split);
      assert(grid_pos < NUM_SUBNODES);

      return std::get<SUBNODES>(m_content)[grid_pos].insert(pos, idx, all_pos);
    }
  }

  constexpr auto find(const Point<Float>& pos) const noexcept -> std::vector<size_t> {
    assert(m_extend.contains_point(pos));

    if (m_is_leaf) {
      return std::get<INDICES>(m_content);
    } else {
      const auto x_split = m_extend.x + m_extend.w / static_cast<Float>(2);
      const auto y_split = m_extend.y + m_extend.h / static_cast<Float>(2);

      const size_t grid_pos = 2ul * (pos.y > y_split) + 1ul * (pos.x > x_split);
      assert(grid_pos < NUM_SUBNODES);

      return std::get<SUBNODES>(m_content)[grid_pos].find(pos);
    }
  }

  constexpr auto find(const Box<Float>& box) const noexcept -> std::vector<size_t> {
    assert(m_extend.intersects(box));

    if (m_is_leaf) {
      return std::get<INDICES>(m_content);
    } else {
      std::vector<size_t> idxs{};
      for (const auto& subnode : std::get<SUBNODES>(m_content)) {
        if (subnode.m_extend.intersects(box)) {
          const auto tmp_idxs = subnode.find(box);
          idxs.insert(std::end(idxs), std::begin(tmp_idxs), std::end(tmp_idxs));
        }
      }
      return idxs;
    }
  }

  constexpr void print(size_t indent = 0ul) const noexcept {
    if (m_is_leaf) {
      std::cout << std::string(indent, ' ') << '[';
      for (auto idx : std::get<INDICES>(m_content)) {
        std::cout << idx << ", ";
      }
      std::cout << "]\n";
    } else {
      for (const auto& subnode : std::get<SUBNODES>(m_content)) {
        subnode.print(indent + 2ul);
        ;
      }
    }
  }
};

template <typename Data, std::floating_point Float = double>
class Quadtree {
  std::vector<Point<Float>> m_pos{};
  std::vector<Data> m_data{};

  Box<Float> m_bounding_box{};

  QuadtreeNode<Float> m_root;

 public:
  constexpr Quadtree(Box<Float> bounding_box)
      : m_bounding_box(std::move(bounding_box)),
        m_root(QuadtreeNode<Float>(m_bounding_box)) {}

  constexpr auto insert(Point<Float> pos, Data data) noexcept -> bool {
    if (!m_bounding_box.contains_point(pos)) {
      return false;
    }

    // TODO: Assumes the pos is not in quadtree
    m_pos.push_back(std::move(pos));
    m_data.push_back(std::move(data));
    assert(m_pos.size() == m_data.size() && m_pos.size() >= 1ul);
    const auto idx = m_pos.size() - 1ul;

    m_root.insert(m_pos[idx], idx, m_pos);

    return true;
  }

  constexpr auto find(const Point<Float>& pos) -> Data& {
    using namespace std::string_literals;
    if (!m_bounding_box.contains_point(pos)) {
      throw std::runtime_error(QT_ERROR_LOC() + ": Position {"s + std::to_string(pos.x) + ", "s +
                               std::to_string(pos.y) + "} is not in bounding_box {["s +
                               std::to_string(m_bounding_box.x) + ", "s +
                               std::to_string(m_bounding_box.x + m_bounding_box.w) + "], ["s +
                               std::to_string(m_bounding_box.y) + ", "s +
                               std::to_string(m_bounding_box.y + m_bounding_box.h) + "]}."s);
    }

    const auto possible_idxs = m_root.find(pos);
    for (size_t idx : possible_idxs) {
      if (pos == m_pos[idx]) {
        return m_data[idx];
      }
    }
    throw std::runtime_error(QT_ERROR_LOC() + ": Position {"s + std::to_string(pos.x) + ", "s +
                             std::to_string(pos.y) + "} is not in quadtree."s);
  }

  constexpr auto find(const Box<Float>& box) const -> std::vector<Data> {
    using namespace std::string_literals;
    if (!m_bounding_box.intersects(box)) {
      throw std::runtime_error(QT_ERROR_LOC() + ": Search box {["s + std::to_string(box.x) + ", "s +
                               std::to_string(box.x + box.w) + "], ["s + std::to_string(box.y) +
                               ", "s + std::to_string(box.y + box.h) +
                               "]} does not intersect bounding_box {["s +
                               std::to_string(m_bounding_box.x) + ", "s +
                               std::to_string(m_bounding_box.x + m_bounding_box.w) + "], ["s +
                               std::to_string(m_bounding_box.y) + ", "s +
                               std::to_string(m_bounding_box.y + m_bounding_box.h) + "]}."s);
    }

    const auto possible_idxs = m_root.find(box);

    std::vector<Data> res{};
    res.reserve(possible_idxs.size());
    for (size_t idx : possible_idxs) {
      if (box.contains_point(m_pos[idx])) {
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

#endif  // QT_QUADTREE_HPP_
