#include <chrono>
#include <iostream>
#include <random>

#include "Quadtree.hpp"

auto main(int argc, char** argv) -> int {
  using Float = float;

  if (argc < 2) {
    std::cerr << "Usage: " << *argv << " <n>\n";
    std::exit(1);
  }

  const size_t n = [cstr = argv[1]] {
    try {
      return std::stoul(cstr);
    } catch (const std::exception&) {
      std::cerr << "Could not parse string `" << cstr << "` to unsigned long.\n";
      std::exit(1);
    }
  }();

  std::cout << "n = " << n << "\n\n\n";

  qtree::Box<Float> bb{.x = 0.0, .y = 0.0, .w = 100.0, .h = 150.0};
  // qtree::Quadtree<size_t, Float, 100UL> qt(bb);
  qtree::Quadtree<std::string, Float, 100UL> qt(bb);

  std::default_random_engine gen(std::random_device{}());
  std::uniform_real_distribution<Float> dist_x(bb.x, bb.x + bb.w);
  std::uniform_real_distribution<Float> dist_y(bb.y, bb.y + bb.h);

  std::vector<qtree::Point<Float>> positions(n);
  std::generate(std::begin(positions), std::end(positions), [&] {
    return qtree::Point{.x = dist_x(gen), .y = dist_y(gen)};
  });

  {
    const auto t_begin = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i) {
      const auto was_inserted = qt.insert(positions[i], std::to_string(i));
      assert(was_inserted);
    }
    const auto t_dur =
        std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_begin);
    std::cout << "Insertion took " << t_dur << "\n\n\n";
  }
  // qt.print();
  // qt.print_root();

  try {
    const auto t_begin = std::chrono::high_resolution_clock::now();
    bool all_correct   = true;
    for (size_t i = 0; i < n; ++i) {
      const auto x = qt.find(positions[i]);
      all_correct  = all_correct && (x == std::to_string(i));
    }
    const auto t_dur =
        std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_begin);
    std::cout << "Found all = " << std::boolalpha << all_correct << '\n';
    std::cout << "Finding all took " << t_dur << "\n\n\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  try {
    const auto x =
        qt.find(qtree::Point{.x = static_cast<Float>(-0.15), .y = static_cast<Float>(15000)});
    std::cout << "x = " << x << '\n';
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n\n\n";
  }

  try {
    const auto x =
        qt.find(qtree::Point{.x = static_cast<Float>(0.15), .y = static_cast<Float>(0.15)});
    std::cout << "x = " << x << '\n';
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n\n\n";
  }

  try {
    qtree::Box box{.x = static_cast<Float>(-15.0),
                   .y = static_cast<Float>(-23.4),
                   .w = static_cast<Float>(1.0),
                   .h = static_cast<Float>(2.5)};
    const auto xs = qt.find(box);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n\n\n";
  }

  try {
    qtree::Circle circle{
        .x = static_cast<Float>(-15.0),
        .y = static_cast<Float>(-23.4),
        .r = static_cast<Float>(2.5),
    };
    const auto xs = qt.find(circle);
  } catch (const std::exception& e) {
    std::cerr << e.what() << "\n\n\n";
  }

  try {
    qtree::Box box{.x = static_cast<Float>(15.0),
                   .y = static_cast<Float>(23.4),
                   .w = static_cast<Float>(1.0),
                   .h = static_cast<Float>(2.5)};
    auto t_begin  = std::chrono::high_resolution_clock::now();
    const auto xs = qt.find(box);
    const auto dur_qt_find =
        std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_begin);

    const auto& ps = qt.pos();
    t_begin        = std::chrono::high_resolution_clock::now();
    const auto naive_count =
        std::count_if(std::cbegin(ps), std::cend(ps), [&](const qtree::Point<Float>& p) {
          return box.contains(p);
        });
    assert(naive_count >= 0);
    const auto dur_naive_find =
        std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_begin);

    std::cout << "Search in box:\n";
    std::cout << "xs.size() = " << xs.size() << '\n';
    std::cout << "naive_count = " << naive_count << '\n';

    std::cout << "Correct count = " << std::boolalpha
              << (xs.size() == static_cast<size_t>(naive_count)) << '\n';
    std::cout << "Quadtree find took " << dur_qt_find << '\n';
    std::cout << "Naive find took " << dur_naive_find << '\n';
    std::cout << "Speedup: " << dur_naive_find.count() / dur_qt_find.count() << "\n\n\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }

  try {
    qtree::Circle circle{
        .x = static_cast<Float>(15.0),
        .y = static_cast<Float>(23.4),
        .r = static_cast<Float>(2.5),
    };
    auto t_begin  = std::chrono::high_resolution_clock::now();
    const auto xs = qt.find(circle);
    const auto dur_qt_find =
        std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_begin);

    const auto& ps = qt.pos();
    t_begin        = std::chrono::high_resolution_clock::now();
    const auto naive_count =
        std::count_if(std::cbegin(ps), std::cend(ps), [&](const qtree::Point<Float>& p) {
          return circle.contains(p);
        });
    assert(naive_count >= 0);
    const auto dur_naive_find =
        std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_begin);

    std::cout << "Search in circle:\n";
    std::cout << "xs.size() = " << xs.size() << '\n';
    std::cout << "naive_count = " << naive_count << '\n';

    std::cout << "Correct count = " << std::boolalpha
              << (xs.size() == static_cast<size_t>(naive_count)) << '\n';
    std::cout << "Quadtree find took " << dur_qt_find << '\n';
    std::cout << "Naive find took " << dur_naive_find << '\n';
    std::cout << "Speedup: " << dur_naive_find.count() / dur_qt_find.count() << "\n\n\n";
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}
