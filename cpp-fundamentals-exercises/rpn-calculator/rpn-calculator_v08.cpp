#include <fmt/core.h>

#include <algorithm>
#include <charconv>
#include <exception>
#include <iostream>
#include <iterator>
#include <locale>
#include <optional>
#include <stack>
#include <stdexcept>
#include <string>
#include <system_error>

/// The set of allowed operators.
constexpr std::string OPERATORS = "+-*/%";

/// Calculator states.
enum class States {
  Operand1, ///< Expecting operand 1.
  Operand2, ///< Expecting operand 2.
  Operator  ///< Expecting operator.
};

/// Input token representation.
struct Token {
  /// Token type.
  enum class Type {
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

///
/// Perform a calculation given two input values and an operator.
///
/// \note There is no overflow handling in place!
///
/// \param lhs Left-hand side input value.
/// \param lhs Right-hand side input value.
/// \param lhs Operator.
///
/// \returns Calculation result.
///
/// \throws An exception if an unsupported operator is specified.
///
[[nodiscard]] static long calculate(long lhs, long rhs, char op) {
  switch (op) {
  case '+': return lhs + rhs;
  case '-': return lhs - rhs;
  case '*': return lhs * rhs;
  case '/': return lhs / rhs;
  case '%': return lhs % rhs;
  default: throw std::invalid_argument{"unsupported operator"};
  }
}

/// The stack memory type.
using Memory = std::stack<long>;

int main() {
  try {
    States s = States::Operand1;
    Memory m;

    while (true) {
      const auto t = read_token();
      if (!t) {
        break; // Assume program stop.
      }

      switch (s) {
      case States::Operand1:
        switch (t->type) {
        case Token::Type::Operand:
          m.push(t->parse());
          s = States::Operand2;
          break;
        case Token::Type::Operator: throw std::runtime_error{"expected operand 1, got operator"};
        case Token::Type::Invalid: throw std::runtime_error{"expected operand 1, got invalid token"};
        }

        break;

      case States::Operand2:
        switch (t->type) {
        case Token::Type::Operand:
          m.push(t->parse());
          s = States::Operator;
          break;
        case Token::Type::Operator: throw std::runtime_error{"expected operand 2, got operator"};
        case Token::Type::Invalid: throw std::runtime_error{"expected operand 2, got invalid token"};
        }

        break;

      case States::Operator:
        switch (t->type) {
        case Token::Type::Operator:
          {
            if (m.size() != 2) {
              throw std::logic_error{"expected two elements in memory"};
            }

            const auto rhs = m.top();
            m.pop();
            const auto lhs = m.top();
            m.pop();

            std::cout << calculate(lhs, rhs, t->value[0]) << '\n';
          }

          s = States::Operand1;
          break;
        case Token::Type::Operand: throw std::runtime_error{"expected operator, got operand"};
        case Token::Type::Invalid: throw std::runtime_error{"expected operator, got invalid token"};
        }

        break;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << '\n';
  }
}
