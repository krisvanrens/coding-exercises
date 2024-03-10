extern "C" {
#include <curses.h>
}

#include <algorithm>
#include <array>
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
    return !is_oob(x, y) && format.at(((width + 1) * static_cast<unsigned int>(y)) + static_cast<unsigned int>(x)) == '#';
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

        for (unsigned int y = 0; y < s.height; y++) {
          if (y < dist_ceiling) {
            s.print(x, y, " "); // Ceiling.
          } else if (y > dist_ceiling && y <= dist_floor) {
            if (bound) {
              s.print(x, y, "\u2591"); // Wall bound.
            } else {
              s.print(x, y, [&] { // Wall.
                if (dist_wall < (MAX_DEPTH * 0.25f)) {
                  return "\u2588";
                } else if (dist_wall < (MAX_DEPTH * 0.5f)) {
                  return "\u2593";
                } else if (dist_wall < (MAX_DEPTH * 0.75f)) {
                  return "\u2592";
                } else if (dist_wall < MAX_DEPTH) {
                  return "\u2591";
                } else {
                  return " ";
                }
              }());
            }
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

      s.update();

      switch (s.get_key()) {
        using enum Screen::Key;
      case Up:
        p.move_up();
        if (MAP.is_wall(static_cast<int>(p.x), static_cast<int>(p.y))) {
          p.move_down(); // Collision detection: undo the previous operation.
        }
        break;
      case Down:
        p.move_down();
        if (MAP.is_wall(static_cast<int>(p.x), static_cast<int>(p.y))) {
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
