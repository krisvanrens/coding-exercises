#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <locale>
#include <string>

/// The set of allowed operators.
constexpr std::string OPERATORS = "+-*/%";

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

  const auto is_digit = [&](char& c) { return std::isdigit(c, loc); };

  if (std::ranges::all_of(input, is_digit)) {
    std::cout << "Got positive number.\n";
  } else if ((input.length() > 1) && (input.starts_with('-')) && std::all_of(std::next(input.begin()), input.end(), is_digit)) {
    std::cout << "Got negative number.\n";
  } else if ((input.length() == 1) && std::ranges::any_of(OPERATORS, [&](const char& c) { return input[0] == c; })) {
    std::cout << "Got operator.\n";
  } else {
    std::cout << "Got invalid input.\n";
  }
}
