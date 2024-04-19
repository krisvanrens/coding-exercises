extern "C" {
#include <curses.h>
}

#include <fmt/core.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <clocale>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace helpers {

template<std::size_t Offset, std::size_t... Is>
constexpr std::index_sequence<(Offset + Is)...> add_offset(std::index_sequence<Is...>) {
  return {};
}

template<std::size_t Offset, std::size_t N>
constexpr auto make_index_sequence_with_offset() {
  return add_offset<Offset>(std::make_index_sequence<N>{});
}

/// Generate an array with offset indexes as values, at compile-time.
template<typename T, std::size_t N, std::size_t Offset>
constexpr auto make_array_with_indices() {
  return []<std::size_t... Is>(std::index_sequence<Is...>) {
    return std::array<T, N>{Is...};
  }
  (make_index_sequence_with_offset<Offset, N>());
}

} // namespace helpers

constexpr int          WALL_COLOR_X          = 10; // Black/background.
constexpr unsigned int NUMBER_OF_WALL_SHADES = 16;
constexpr auto         WALL_SHADES           = helpers::make_array_with_indices<int, NUMBER_OF_WALL_SHADES, 11>(); // 11, 12, 13, ...

constexpr float PI        = std::numbers::pi_v<float>;
constexpr float PI2       = PI * 2.0f;
constexpr float FOV       = PI / 3.0f; // Field of view in [radians].
constexpr float MAX_DEPTH = 15.0f;     // Maximum visible depth in [map block units].

/// Wrapper around the default 'stdscr' window in ncurses.
struct Screen {
private:
  const WINDOW* const window_;

public:
  enum class Key { Up, Down, Left, Right, Quit, Other };

  Screen()
    : window_{initscr()}
    , width{static_cast<unsigned int>(getmaxx(stdscr))}
    , height{static_cast<unsigned int>(getmaxy(stdscr))} {
    cbreak();    // Break on character input (i.e. don't wait for enter).
    noecho();    // Don't echo input keys.
    curs_set(0); // Disable cursor.

    if (!window_) {
      throw std::runtime_error{"failed to initialize screen"};
    }

    // Uncomment this line to enable delay-less operation of ncurses. Otherwise ncurses will blocking-wait for key input.
    // nodelay(stdscr, TRUE);

    if (has_colors() == FALSE) {
      throw std::runtime_error{"your terminal does not support color"};
    }

    start_color();

    init_color(COLOR_BLACK, 0, 0, 0); // Reinitialize black to be really dark.
    init_pair(WALL_COLOR_X, COLOR_BLACK, COLOR_BLACK);

    // Note: we overlap the IDs for colors and color pairs. Not as ncurses intended, but OK for this example.
    for (unsigned int i = 0; i < WALL_SHADES.size(); i++) {
      const int v     = 1000 - static_cast<int>(i * (1000 / WALL_SHADES.size()));
      const int shade = WALL_SHADES.at(i);
      init_extended_color(shade, v, v, v);
      init_extended_pair(shade, shade, COLOR_BLACK);
    }

    // Override default foreground/background colors as white on black.
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    attron(COLOR_PAIR(1));
  }

  ~Screen() {
    endwin();
  }

  /// Write console buffer to screen.
  void update() {
    refresh();
  }

  /// Print string to specific coordinates in console buffer.
  void print(unsigned int x, unsigned int y, std::string_view s) const {
    mvaddstr(static_cast<int>(y), static_cast<int>(x), s.data());
  }

  /// Capture input key.
  [[nodiscard]] Key get_key() const {
    switch (getch()) {
    case 'w': return Key::Up;
    case 's': return Key::Down;
    case 'a': return Key::Left;
    case 'd': return Key::Right;
    case 'q': return Key::Quit;
    default: return Key::Other;
    }
  }

  const unsigned int width;
  const unsigned int height;
};

/// Abstraction over a rectangular ASCII art level map definition.
struct LevelMap {
  /// Constructor. Takes an ASCII art map definition where '#' are walls.
  explicit LevelMap(std::string&& format_)
    : format{std::move(format_)}
    , width{static_cast<unsigned int>(format.find_first_of('\n'))}
    , height{static_cast<unsigned int>(format.find_last_of('\n')) / width} {
    if (width < 3 || height < 3) {
      throw std::invalid_argument{"invalid level dimensions -- must at least be 3x3 units"};
    }

    if ((width + 1) * height != format.size()) {
      throw std::invalid_argument{"invalid level dimensions -- must be rectangular"};
    }
  }

  /// Check if a coordinate on the map is out-of-bounds (OOB).
  [[nodiscard]] bool is_oob(int x, int y) const {
    return x < 0 || y < 0 || x >= static_cast<int>(width) || y >= static_cast<int>(height);
  }

  /// Check if a coordinate on the map is a wall element.
  [[nodiscard]] bool is_wall(int x, int y) const {
    return !is_oob(x, y) && format.at((width + 1) * static_cast<unsigned int>(y) + static_cast<unsigned int>(x)) == '#';
  }

  const std::string  format;
  const unsigned int width;
  const unsigned int height;
};

/// Player state manager.
struct Player {
  Player(float x, float y, float a)
    : x{x}
    , y{y}
    , angle{a} {
  }

  void move_up() {
    x += 0.1f * std::sin(angle);
    y += 0.1f * std::cos(angle);
  }

  void move_down() {
    x -= 0.1f * std::sin(angle);
    y -= 0.1f * std::cos(angle);
  }

  void turn_ccw() {
    angle = std::fmod(angle - 0.1f + PI2, PI2);
  }

  void turn_cw() {
    angle = std::fmod(angle + 0.1f, PI2);
  }

  float x;     // Current X-coordinate in [map block units].
  float y;     // Current Y-coordinate in [map block units].
  float angle; // Current orientation angle in [radians].
};

[[nodiscard]] static constexpr int distance_to_wall_shade(float d) {
  if (d < MAX_DEPTH) {
    const float shade = std::clamp(MAX_DEPTH - (2.0f * d), 0.0f, MAX_DEPTH);
    return WALL_SHADES.at(WALL_SHADES.size() - 1 - static_cast<std::size_t>(shade * (WALL_SHADES.size() / MAX_DEPTH)));
  } else {
    return WALL_COLOR_X;
  }
}

[[nodiscard]] static constexpr std::string angle_to_char(float a) {
  constexpr float D = PI / 8.0f;

  if (a > (PI2 - D) || a <= D) {
    return "\u21D3"; // Downwards arrow.
  } else if (a > D && a <= (D * 3.0f)) {
    return "\u21D8"; // South East arrow.
  } else if (a > (D * 3.0f) && a <= (D * 5.0f)) {
    return "\u21D2"; // Rightwards arrow.
  } else if (a > (D * 5.0f) && a <= (PI - D)) {
    return "\u21D7"; // North East arrow.
  } else if (a > (PI - D) && a <= (PI + D)) {
    return "\u21D1"; // Upwards arrow.
  } else if (a > (PI + D) && a <= (PI + (D * 3.0f))) {
    return "\u21D6"; // North West arrow.
  } else if (a > (PI + (D * 3.0f)) && a <= (PI + (D * 5.0f))) {
    return "\u21D0"; // Leftwards arrow.
  } else {
    return "\u21D9"; // South West arrow.
  }
}

int main() {
  try {
    if (std::setlocale(LC_ALL, "") == nullptr) { // Required for Unicode support.
      throw std::runtime_error{"failed to set locale"};
    }

    const LevelMap MAP{"####################\n"
                       "#   ##             #\n"
                       "#   ##             #\n"
                       "#                  #\n"
                       "#         ##########\n"
                       "#                  #\n"
                       "######             #\n"
                       "#    #      ###    #\n"
                       "#    #      ###    #\n"
                       "#                  #\n"
                       "#                  #\n"
                       "####################\n"};

    Screen s;
    Player p{7.0f, 1.0f, 0.0f};

    while (true) {
      const auto t_start = std::chrono::system_clock::now();

      // Display mini-map and player location / orientation.
      s.print(0, 0, MAP.format);
      s.print(static_cast<unsigned int>(p.x), static_cast<unsigned int>(p.y), angle_to_char(p.angle));

      for (unsigned int x = 0; x < s.width; x++) {
        const float ray_angle = p.angle - (FOV / 2) + (static_cast<float>(x) * FOV) / static_cast<float>(s.width);
        const float norm_x    = std::sin(ray_angle);
        const float norm_y    = std::cos(ray_angle);

        float dist_wall = 0.0f;
        bool  hit       = false; // Indicates 'ray hit'.
        bool  bound     = false; // Indicates wall block boundary.
        while (!hit && (dist_wall < MAX_DEPTH)) {
          dist_wall += 0.1f;

          const int xx = static_cast<int>(std::round(p.x + norm_x * dist_wall));
          const int yy = static_cast<int>(std::round(p.y + norm_y * dist_wall));

          const bool hit_wall = MAP.is_wall(xx, yy);
          hit                 = MAP.is_oob(xx, yy) || hit_wall;

          if (hit_wall) {
            std::array<std::pair<float, float>, 4> corners; // Distances and dot products per wall block corner.

            for (unsigned int tx = 0; tx < 2; tx++) {
              for (unsigned int ty = 0; ty < 2; ty++) {
                const float vx          = static_cast<float>(xx + tx) - p.x;
                const float vy          = static_cast<float>(yy + ty) - p.y;
                const float d           = std::sqrt(vx * vx + vy * vy);
                corners.at(ty * 2 + tx) = std::make_pair(d, (norm_x * vx / d) + (norm_y * vy / d));
              }
            }

            std::ranges::sort(corners, [](const auto& a, const auto& b) { return a.first < b.first; });

            bound = (std::acos(corners.at(0).second) < 0.01f) || (std::acos(corners.at(1).second) < 0.01f);
          }
        }

        const long dist_ceiling = static_cast<long>(std::round((static_cast<float>(s.height) / 2.0f) - (static_cast<float>(s.height) / dist_wall)));
        const long dist_floor   = static_cast<long>(std::round(s.height - dist_ceiling));
        const int  wall_shade   = distance_to_wall_shade(dist_wall);

        for (unsigned int y = 0; y < s.height; y++) {
          if (x >= MAP.width || y >= MAP.height) {
            if (y < dist_ceiling) {
              s.print(x, y, " "); // Ceiling.
            } else if (y > dist_ceiling && y <= dist_floor) {
              attron(COLOR_PAIR(wall_shade));
              if (bound) {
                s.print(x, y, "\u2593"); // Wall bound.
              } else {
                s.print(x, y, "\u2588"); // Wall.
              }
              attroff(COLOR_PAIR(wall_shade));
            } else {
              const float d = 1.0f - ((static_cast<float>(y) - (static_cast<float>(s.height) / 2.0f)) / (static_cast<float>(s.height) / 2.0f));
              s.print(x, y, [&] { // Floor.
                if (d < 0.25f) {
                  return "#";
                } else if (d < 0.5f) {
                  return "x";
                } else if (d < 0.75f) {
                  return "-";
                } else if (d < 0.9f) {
                  return ".";
                } else {
                  return " ";
                }
              }());
            }
          }
        }
      }

      const auto t_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - t_start);
      s.print(0, s.height - 1, fmt::format("Frame rate: {:.0f} FPS", 1e6f / static_cast<float>(t_elapsed.count())));

      s.update();

      switch (s.get_key()) {
        using enum Screen::Key;
      case Up:
        p.move_up();
        if (MAP.is_wall(static_cast<int>(std::round(p.x)), static_cast<int>(std::round(p.y)))) {
          p.move_down(); // Collision detection: undo the previous operation.
        }
        break;
      case Down:
        p.move_down();
        if (MAP.is_wall(static_cast<int>(std::round(p.x)), static_cast<int>(std::round(p.y)))) {
          p.move_up(); // Collision detection: undo the previous operation.
        }
        break;
      case Left: p.turn_ccw(); break;
      case Right: p.turn_cw(); break;
      case Other: break;
      case Quit: return EXIT_SUCCESS;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << '\n';
    return EXIT_FAILURE;
  }
}
