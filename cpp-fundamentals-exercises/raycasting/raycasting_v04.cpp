extern "C" {
#include <curses.h>
}

#include <clocale>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <string>

constexpr float PI        = std::numbers::pi_v<float>;
constexpr float PI2       = PI * 2.0f;
constexpr float FOV       = PI / 3.0f; // Field of view in [radians].
constexpr float MAX_DEPTH = 15.0f;     // Maximum visible depth in [map block units].

int main() {
  if (std::setlocale(LC_ALL, "") == nullptr) { // Required for Unicode support.
    std::cerr << "Error: failed to set locale\n";
    return EXIT_FAILURE;
  }

  const std::string MAP{"####################"
                        "#   ##             #"
                        "#   ##             #"
                        "#                  #"
                        "#         ##########"
                        "#                  #"
                        "######             #"
                        "#    #      ###    #"
                        "#    #      ###    #"
                        "#                  #"
                        "#                  #"
                        "####################"};

  const unsigned int MAP_WIDTH  = 20;
  const unsigned int MAP_HEIGHT = 12;

  initscr();

  const auto SCREEN_WIDTH  = static_cast<unsigned int>(getmaxx(stdscr));
  const auto SCREEN_HEIGHT = static_cast<unsigned int>(getmaxy(stdscr));

  cbreak();    // Break on character input (i.e. don't wait for enter).
  noecho();    // Don't echo input keys.
  curs_set(0); // Disable cursor.

  // Uncomment this line to enable delay-less operation of ncurses. Otherwise ncurses will blocking-wait for key input.
  // nodelay(stdscr, TRUE);

  float player_x     = 7.0f;
  float player_y     = 1.0f;
  float player_angle = 0.0f;

  while (true) {
    for (unsigned int x = 0; x < SCREEN_WIDTH; x++) {
      const float ray_angle = player_angle - (FOV / 2) + (static_cast<float>(x) * FOV) / static_cast<float>(SCREEN_WIDTH);
      const float norm_x    = std::sin(ray_angle);
      const float norm_y    = std::cos(ray_angle);

      float dist_wall = 0.0f;
      bool  hit       = false; // Indicates 'ray hit'.
      while (!hit && (dist_wall < MAX_DEPTH)) {
        dist_wall += 0.1f;

        const int xx = static_cast<int>(std::round(player_x + norm_x * dist_wall));
        const int yy = static_cast<int>(std::round(player_y + norm_y * dist_wall));

        hit = xx < 0 || yy < 0 || xx >= static_cast<int>(MAP_WIDTH) || yy >= static_cast<int>(MAP_HEIGHT)
              || MAP.at((MAP_WIDTH * static_cast<unsigned int>(yy)) + static_cast<unsigned int>(xx)) == '#';
      }

      const long dist_ceiling = static_cast<long>(std::round((static_cast<float>(SCREEN_HEIGHT) / 2.0f) - (static_cast<float>(SCREEN_HEIGHT) / dist_wall)));
      const long dist_floor   = static_cast<long>(std::round(SCREEN_HEIGHT - dist_ceiling));

      for (unsigned int y = 0; y < SCREEN_HEIGHT; y++) {
        if (y < dist_ceiling) {
          mvaddstr(y, x, " "); // Ceiling.
        } else if (y > dist_ceiling && y <= dist_floor) {
          mvaddstr(y, x, [&] { // Wall.
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
        } else {
          const float d = 1.0f - ((static_cast<float>(y) - (static_cast<float>(SCREEN_HEIGHT) / 2.0f)) / (static_cast<float>(SCREEN_HEIGHT) / 2.0f));
          mvaddstr(y, x, [&] { // Floor.
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

    refresh();

    switch (getch()) {
    case 'w':
      player_x += 0.1f * std::sin(player_angle);
      player_y += 0.1f * std::cos(player_angle);

      // Collision detection: undo the previous operation.
      if ((player_x > 0.0f) && (player_y > 0.0f) && (static_cast<unsigned int>(player_x) <= MAP_WIDTH) && (static_cast<unsigned int>(player_y) <= MAP_HEIGHT)
          && MAP.at((MAP_WIDTH * static_cast<unsigned int>(player_y)) + static_cast<unsigned int>(player_x)) == '#') {
        player_x -= 0.1f * std::sin(player_angle);
        player_y -= 0.1f * std::cos(player_angle);
      }
      break;
    case 's':
      player_x -= 0.1f * std::sin(player_angle);
      player_y -= 0.1f * std::cos(player_angle);

      // Collision detection: undo the previous operation.
      if ((player_x > 0.0f) && (player_y > 0.0f) && (static_cast<unsigned int>(player_x) <= MAP_WIDTH) && (static_cast<unsigned int>(player_y) <= MAP_HEIGHT)
          && MAP.at((MAP_WIDTH * static_cast<unsigned int>(player_y)) + static_cast<unsigned int>(player_x)) == '#') {
        player_x += 0.1f * std::sin(player_angle);
        player_y += 0.1f * std::cos(player_angle);
      }
      break;
    case 'a': player_angle = std::fmod(player_angle - 0.1f + PI2, PI2); break;
    case 'd': player_angle = std::fmod(player_angle + 0.1f, PI2); break;
    case 'q': endwin(); return EXIT_SUCCESS;
    default: break;
    }
  }
}
