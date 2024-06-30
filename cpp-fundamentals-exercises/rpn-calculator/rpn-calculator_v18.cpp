#include <fmt/core.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <locale>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <variant>

#include "stack.hpp"

/// Helper class to let function overloading deal with type selection.
template<typename... Ts>
struct overload : Ts... {
  using Ts::operator()...;
};

/// Class template argument (CTAD) deduction guide, not needed for C++20 and later.
// template<typename... Ts> overload(Ts...) -> overload<Ts...>;

/// The set of allowed operators.
constexpr std::string OPERATORS = "+-*/%";

/// Calculation-related specific error type.
class calculation_error final : public std::exception {
public:
  explicit calculation_error(std::string_view message)
    : message_{message} {
  }

  [[nodiscard]] const char* what() const noexcept override {
    return message_.c_str();
  }

private:
  std::string message_;
};

namespace States {

/// Expecting operand 1.
struct Operand1 {};

/// Expecting operand 2.
struct Operand2 {};

/// Expecting operator.
struct Operator {};

/// Show result.
struct Result {};

} // namespace States

/// State representation.
using State = std::variant<States::Operand1, States::Operand2, States::Operator, States::Result>;

/// Any signed arithmetic type.
template<typename T>
concept signed_arithmetic = std::is_signed_v<T> && std::is_arithmetic_v<T>;

namespace Tokens {

/// Operand token.
struct Operand {
  const std::string value;

  ///
  /// Parse token to a value type indicated by the template argument.
  ///
  /// \returns The parsed value.
  ///
  /// \throws An exception when a parse error occurs, or this function is called on an empty value.
  ///
  template<signed_arithmetic T>
  [[nodiscard]] T parse() const {
    if (!value.empty()) {
      //
      // NOTE: Select the 'long double' overload of from_chars for maximum value width. Depending on the platform for
      //        which this code is compiled, it will provide 80 bits or even 128 bits extended floating-point precision.
      //        For MSVC this may not even have any effect and will still use 64 bits, like 'double'.
      //
      long double v{};
      const auto [ptr, error] = std::from_chars(value.data(), value.data() + value.size(), v);
      if (error == std::errc{} && std::string{ptr}.empty()) {
        // Check for invalid cross-type parse requests.
        if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T>) {
          if (std::fmod(v, 1.0) > std::numeric_limits<double>::epsilon()) {
            throw std::logic_error{fmt::format("failed to parse input '{}': invalid cross-type parse", value)};
          }
        }

        // Check for overflow errors.
        if (v > static_cast<double>(std::numeric_limits<T>::max()) || v < static_cast<double>(std::numeric_limits<T>::lowest())) {
          throw calculation_error{fmt::format("failed to parse input '{}': parse type value overflow", value)};
        }

        return static_cast<T>(v);
      } else {
        throw calculation_error{fmt::format("failed to parse input '{}'", value)};
      }
    }

    throw std::logic_error{"trying to call parse on an empty value"};
  }
};

/// Operator token.
struct Operator {
  const char op;
};

/// End-of-calculation token.
struct Eoc {};

} // namespace Tokens

/// Input token representation.
using Token = std::variant<Tokens::Operand, Tokens::Operator, Tokens::Eoc>;

namespace {

///
/// Read a token from standard input.
///
/// \returns The read token.
///
/// \throws A `std::runtime_error` if input stream reading fails.
///
[[nodiscard]] Token read_token() {
  static const auto loc = std::locale("en_US.UTF-8");

  std::string input;
  std::cin >> input;

  if (!std::cin.good() && !std::cin.eof()) {
    throw std::runtime_error{"failed to read standard input stream"};
  }

  if (std::cin.eof()) { // Handles Ctrl-D as well.
    return Tokens::Eoc{};
  }

  // TODO: Alternative approach:
  // std::getline(std::cin, input);

  if ((input.length() == 1) && std::ranges::any_of(OPERATORS, [&](const char& c) { return input[0] == c; })) {
    return Tokens::Operator{input[0]};
  } else {
    return Tokens::Operand{input};
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
template<typename T>
[[nodiscard]] T calculate(T lhs, T rhs, char op) {
  switch (op) {
  case '+': return lhs + rhs;
  case '-': return lhs - rhs;
  case '*': return lhs * rhs;
  case '/':
    if (rhs == 0) {
      throw calculation_error{"division by zero"};
    }

    return lhs / rhs;
  case '%':
    if constexpr (!std::is_floating_point_v<T>) {
      if (rhs == 0) {
        throw calculation_error{"division by zero"};
      }

      return lhs % rhs;
    }
  default: throw std::invalid_argument{"unsupported operator"};
  }
}

} // namespace

/// The stack memory type.
using Memory = Stack<float, 2>;

int main() {
  try {
    bool   stop         = false;
    bool   got_operator = false;
    State  s            = States::Operand1{};
    Memory m;

    while (!stop) {
      const Token t = read_token();

      try {
        // clang-format off
        std::visit(overload{
          [&](States::Operand1&) {
            std::visit(overload{
              [&](const Tokens::Operand& o) {
                m.push(o.parse<float>());
                s = States::Operand2{};
              },
              [](const Tokens::Operator&) { throw calculation_error{"expected operand 1, got operator"};           },
              [](const Tokens::Eoc&)      { throw calculation_error{"expected operand 1, got end-of-calculation"}; },
            }, t);
          },
          [&](States::Operand2&) {
            std::visit(overload{
              [&](const Tokens::Operand& o) {
                m.push(o.parse<float>());
                s = States::Operator{};
              },
              [&](const Tokens::Eoc&) {
                if (got_operator) {
                  s = States::Result{};
                } else {
                  throw calculation_error{"expected operand 2, got end-of-calculation"};
                }
              },
              [](const Tokens::Operator&) { throw calculation_error{"expected operand 2, got operator"}; },
            }, t);
          },
          [&](States::Operator&) {
            std::visit(overload{
              [&](const Tokens::Operator& o) {
                if (m.size() != 2) {
                  throw std::logic_error{"expected two elements in memory"};
                }

                const auto rhs = m.pop().value();
                const auto lhs = m.pop().value();
                m.push(calculate(lhs, rhs, o.op));

                got_operator = true;
                s = States::Operand2{};
              },
              [](const Tokens::Operand&) { throw calculation_error{"expected operator, got operand"};            },
              [](const Tokens::Eoc&)     { throw calculation_error{"expected operator, got end-of-calculation"}; },
            }, t);
          },
          [&](States::Result&) {
            if (m.size() != 1) {
              throw std::logic_error{"expected only a single result in memory"};
            }

            std::cout << m.pop().value() << '\n';

            stop = true; // Bail out.
          }
        }, s);
        // clang-format on
      } catch (const calculation_error& e) {
        std::cout << "Error: " << e.what() << '\n';
        stop = true;
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Caught exception: " << e.what() << '\n';
  }
}
