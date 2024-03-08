#include <algorithm>
#include <exception>
#include <iostream>
#include <iterator>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string>

/// The set of allowed operators.
constexpr std::string OPERATORS = "+-*/%";

/// Input token representation.
struct Token {
  /// Token type.
  enum class Type {
    Operand,  ///< Any valid operand (any arithmetic number).
    Operator, ///< Any valid operator.
    Invalid   ///< Invalid / unknown token.
  } type;

  std::string value;
};

///
/// Read a token from standard input.
///
/// \returns The read token, or an empty optional upon EOF.
///
/// \throws A `std::runtime_error` if input stream reading fails.
///
[[nodiscard]] static std::optional<Token> read_token() {
  static const auto loc = std::locale("en_US.UTF-8");

  std::string input;
  std::cin >> input;

  if (!std::cin.good() && !std::cin.eof()) {
    throw std::runtime_error{"failed to read standard input stream"};
  }

  if (std::cin.eof()) { // Handles Ctrl-D as well.
    return std::nullopt;
  }

  // TODO: Alternative approach:
  // std::getline(std::cin, input);

  const auto is_digit = [&](char& c) { return std::isdigit(c, loc); };

  if (std::ranges::all_of(input, is_digit)) {
    return Token{.type = Token::Type::Operand, .value = input}; // Positive number.
  } else if ((input.length() > 1) && (input.starts_with('-')) && std::all_of(std::next(input.begin()), input.end(), is_digit)) {
    return Token{.type = Token::Type::Operand, .value = input}; // Negative number.
  } else if ((input.length() == 1) && std::ranges::any_of(OPERATORS, [&](const char& c) { return input[0] == c; })) {
    return Token{.type = Token::Type::Operator, .value = input}; // Negative number.
  } else {
    return Token{.type = Token::Type::Invalid, .value = input}; // Negative number.
  }
}

int main() {
  try {
    if (const auto t = read_token(); t) {
      std::cout << "Got token with value '" << t->value << "' and type: " << [&] {
        switch (t->type) {
        case Token::Type::Operand: return "operand";
        case Token::Type::Operator: return "operator";
        case Token::Type::Invalid: return "invalid";
        default: throw std::logic_error{"unsupported token type"};
        }
      }() << ".\n";
    } else {
      std::cout << "Did not get any token.\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << '\n';
  }
}
