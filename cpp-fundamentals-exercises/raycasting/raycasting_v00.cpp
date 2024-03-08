extern "C" {
#include <curses.h>
}

#include <clocale>
#include <cstdlib>
#include <iostream>

int main() {
  if (std::setlocale(LC_ALL, "") == nullptr) { // Required for Unicode support.
    std::cerr << "Error: failed to set locale\n";
    return EXIT_FAILURE;
  }

  initscr();

  const unsigned int SCREEN_WIDTH  = static_cast<unsigned int>(getmaxx(stdscr));
  const unsigned int SCREEN_HEIGHT = static_cast<unsigned int>(getmaxy(stdscr));

  cbreak();    // Break on character input (i.e. don't wait for enter).
  noecho();    // Don't echo input keys.
  curs_set(0); // Disable cursor.

  // Uncomment this line to enable delay-less operation of ncurses. Otherwise ncurses will blocking-wait for key input.
  // nodelay(stdscr, TRUE);

  mvprintw(0, 0, "The screen size is: %ux%u characters", SCREEN_WIDTH, SCREEN_HEIGHT);

  refresh();
  getch(); // Wait for a key press.
  endwin();
}
