#include <iostream>
#include <locale>
#include <string>

int main() {
  const auto loc = std::locale("en_US.UTF-8");

  std::string input;
  std::cin >> input;

  if (!std::cin.good() && !std::cin.eof()) {
    std::cerr << "Error: Failed to read standard input stream.\n";
  }

  if (std::cin.eof()) { // Handles Ctrl-D as well.
    std::cout << "Got EOF!\n";
    return 0;
  }

  // TODO: Alternative approach:
  // std::getline(std::cin, input);

  std::cout << "Got input: '" << input << "'\n";
}
