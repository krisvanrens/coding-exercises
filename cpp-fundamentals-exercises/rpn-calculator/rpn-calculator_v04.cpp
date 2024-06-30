#include <fmt/core.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <iterator>
#include <locale>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>

/// The set of allowed operators.
constexpr std::string OPERATORS = "+-*/%";

/// Input token representation.
struct Token {
  /// Token type.
  enum class Type : uint8_t {
    Operand,  ///< Any valid operand (any arithmetic number).
    Operator, ///< Any valid operator.
    Invalid   ///< Invalid / unknown token.
  } type;

  std::string value;

  ///
  /// Parse token to a long integer.
  ///
  /// \returns The parsed value.
  ///
  /// \throws An exception when a parse error occurs, or this function is called on an empty value.
  ///
  [[nodiscard]] long parse() const {
    if (!value.empty()) {
      long v{};
      const auto [ptr, error] = std::from_chars(value.data(), value.data() + value.size(), v);
      if (error == std::errc{}) {
        return v;
      } else {
        throw std::runtime_error{fmt::format("failed to parse input '{}'", value)};
      }
    }

    throw std::logic_error{"trying to call parse on an empty value"};
  }
};

namespace {

///
/// Read a token from standard input.
///
/// \returns The read token, or an empty optional upon EOF.
///
/// \throws A `std::runtime_error` if input stream reading fails.
///
[[nodiscard]] std::optional<Token> read_token() {
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

} // namespace

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

      if (t->type == Token::Type::Operand) {
        std::cout << "Token operand value parses to: " << t->parse() << '\n';
      }
    } else {
      std::cout << "Did not get any token.\n";
    }
  } catch (const std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << '\n';
  }
}
