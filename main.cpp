#include <iostream>
#include <random>

#include "Quadtree.hpp"

auto main() noexcept -> int {
  using Float = float;

  Box<Float> bb{.x = 0.0, .y = 0.0, .w = 1.0, .h = 1.0};
  Quadtree<size_t, Float> qt(bb);

  std::default_random_engine gen(std::random_device{}());
  std::uniform_real_distribution<Float> dist(0.0, 1.0);

  constexpr size_t n = 100;
  std::vector<Point<Float>> positions(n);
  std::generate(std::begin(positions), std::end(positions), [&] {
    return Point{.x = dist(gen), .y = dist(gen)};
  });

  for (size_t i = 0; i < n; ++i) {
    const auto was_inserted = qt.insert(positions[i], i);
    assert(was_inserted);
  }

  qt.print();
  qt.print_root();

  try {
    bool all_correct = true;
    for (size_t i = 0; i < n; ++i) {
      const auto x = qt.find(positions[i]);
      all_correct  = all_correct && (x == i);
    }
    std::cout << "all_correct = " << std::boolalpha << all_correct << '\n';
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  try {
    const auto x = qt.find(Point{.x = static_cast<Float>(0.15), .y = static_cast<Float>(0.15)});
    std::cout << "x = " << x << '\n';
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  try {
    const auto xs = qt.find(Box{.x = static_cast<Float>(0.0),
                                .y = static_cast<Float>(0.6),
                                .w = static_cast<Float>(0.25),
                                .h = static_cast<Float>(0.25)});
    std::cout << "xs.size() = " << xs.size() << '\n';
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}
