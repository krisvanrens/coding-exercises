#include <fmt/core.h>

#ifdef ENABLE_DOCTESTS
#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>
#endif

#include <algorithm>
#include <charconv>
#include <cmath>
#include <exception>
#include <iostream>
#include <istream>
#include <iterator>
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
      if (error == std::errc{}) {
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

/// Invalid token.
struct Invalid {};

} // namespace Tokens

/// Input token representation.
using Token = std::variant<Tokens::Operand, Tokens::Operator, Tokens::Eoc, Tokens::Invalid>;

///
/// Read a token from standard input.
///
/// \returns The read token.
///
/// \throws A `std::runtime_error` if input stream reading fails.
///
[[nodiscard]] static Token read_token(std::istream& source = std::cin) {
  static const auto loc = std::locale("en_US.UTF-8");

  std::string input;
  source >> input;

  if (!source.good() && !source.eof()) {
    throw std::runtime_error{"failed to read input stream"};
  }

  if (input.empty() && source.eof()) { // Handles Ctrl-D as well for std::cin.
    return Tokens::Eoc{};
  }

  // TODO: Alternative approach:
  // std::getline(source, input);

  const auto is_digit = [&](char& c) { return std::isdigit(c, loc); };

  if (std::ranges::all_of(input, is_digit)) {
    return Tokens::Operand{input}; // Positive number.
  } else if ((input.length() > 1) && (input.starts_with('-')) && std::all_of(std::next(input.begin()), input.end(), is_digit)) {
    return Tokens::Operand{input}; // Negative number.
  } else if ((input.length() == 1) && std::ranges::any_of(OPERATORS, [&](const char& c) { return input[0] == c; })) {
    return Tokens::Operator{input[0]};
  } else {
    return Tokens::Invalid{};
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
[[nodiscard]] static T calculate(T lhs, T rhs, char op) {
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

/// The stack memory type.
using Memory = Stack<long, 2>;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
  int result{};

#ifdef ENABLE_DOCTESTS
  doctest::Context ctx;
  ctx.applyCommandLine(argc, argv);
  result = ctx.run();
  if (ctx.shouldExit()) {
    return result;
  }
#endif // ENABLE_DOCTESTS

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
                m.push(o.parse<long>());
                s = States::Operand2{};
              },
              [](const Tokens::Operator&) { throw calculation_error{"expected operand 1, got operator"};           },
              [](const Tokens::Eoc&)      { throw calculation_error{"expected operand 1, got end-of-calculation"}; },
              [](const Tokens::Invalid&)  { throw calculation_error{"expected operand 1, got invalid token"};      }
            }, t);
          },
          [&](States::Operand2&) {
            std::visit(overload{
              [&](const Tokens::Operand& o) {
                m.push(o.parse<long>());
                s = States::Operator{};
              },
              [&](const Tokens::Eoc&) {
                if (got_operator) {
                  s = States::Result{};
                } else {
                  throw calculation_error{"expected operand 2, got end-of-calculation"};
                }
              },
              [](const Tokens::Operator&) { throw calculation_error{"expected operand 2, got operator"};      },
              [](const Tokens::Invalid&)  { throw calculation_error{"expected operand 2, got invalid token"}; }
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
              [](const Tokens::Invalid&) { throw calculation_error{"expected operator, got invalid token"};      }
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

  return result;
}

#ifdef ENABLE_DOCTESTS

/// Helper to suppress compiler errors regarding 'nodiscard'.
#define USE(e) static_cast<void>(e)

TEST_CASE("Tokens::Operand::parse") {
  SUBCASE("Valid input") {
    Tokens::Operand o1{"42"};
    CHECK(o1.parse<int>() == 42);

    Tokens::Operand o2{"-1234567890"};
    CHECK(o2.parse<long>() == -1234567890);

    Tokens::Operand o3{"3.14"};
    CHECK(o3.parse<float>() == doctest::Approx(3.14f));

    Tokens::Operand o4{"2.71828"};
    CHECK(o4.parse<double>() == doctest::Approx(2.71828));
  }

  SUBCASE("Invalid input") {
    Tokens::Operand o1{"abc"};
    CHECK_THROWS_AS(USE(o1.parse<int>()), calculation_error);

    Tokens::Operand o2{"123.45"};
    CHECK_THROWS_AS(USE(o2.parse<long>()), std::logic_error);

    Tokens::Operand o3{"xyz"};
    CHECK_THROWS_AS(USE(o3.parse<double>()), calculation_error);
  }

  SUBCASE("Empty input") {
    Tokens::Operand o{""};
    CHECK_THROWS_AS(USE(o.parse<int>()), std::logic_error);
    CHECK_THROWS_AS(USE(o.parse<long>()), std::logic_error);
    CHECK_THROWS_AS(USE(o.parse<float>()), std::logic_error);
    CHECK_THROWS_AS(USE(o.parse<double>()), std::logic_error);
  }

  SUBCASE("Overflow input") {
    Tokens::Operand o1{"2147483648"};
    CHECK_THROWS_AS(USE(o1.parse<int>()), calculation_error); // Exceeds the range of int.

    Tokens::Operand o2{"92233720368547758080"};
    CHECK_THROWS_AS(USE(o2.parse<long>()), calculation_error); // Exceeds the range of long.
  }
}

TEST_CASE("read_token") {
  SUBCASE("Reads an operand") {
    std::istringstream input{"123"};

    const auto t = read_token(input);

    CHECK(std::holds_alternative<Tokens::Operand>(t));
    CHECK(std::get<Tokens::Operand>(t).value == "123");
  }

  SUBCASE("Reads a negative operand") {
    std::istringstream input{"-456"};

    const auto t = read_token(input);

    CHECK(std::holds_alternative<Tokens::Operand>(t));
    CHECK(std::get<Tokens::Operand>(t).value == "-456");
  }

  SUBCASE("Reads an operator") {
    std::istringstream input{"+"};

    const auto t = read_token(input);

    CHECK(std::holds_alternative<Tokens::Operator>(t));
    CHECK(std::get<Tokens::Operator>(t).op == '+');
  }

  SUBCASE("Reads an invalid token") {
    std::istringstream input{"abc"};

    const auto t = read_token(input);

    CHECK(std::holds_alternative<Tokens::Invalid>(t));
  }

  SUBCASE("Returns empty optional upon EOF") {
    std::istringstream input;
    std::cin.rdbuf(input.rdbuf());

    const auto t = read_token();

    CHECK(std::holds_alternative<Tokens::Eoc>(t));
  }
}

TEST_CASE("calculate") {
  SUBCASE("Addition") {
    CHECK(calculate(2, 3, '+') == 5);
    CHECK(calculate(0, 0, '+') == 0);
    CHECK(calculate(-5, 10, '+') == 5);
    CHECK(calculate(-9223372036854775807L, 1L, '+') == -9223372036854775806L);
    CHECK(calculate(-9223372036854775807L, 1L, '+') == -9223372036854775806L);
    CHECK(calculate(9223372036854775807L, -1L, '+') == 9223372036854775806L);
    CHECK(calculate(0L, 9223372036854775807L, '+') == 9223372036854775807L);
    // CHECK(calculate(-9223372036854775807L, -9223372036854775807L, '+') == -18446744073709551614L); // 64-bit overflow.
    CHECK(calculate(9223372036854775807L, -9223372036854775807L, '+') == 0L);
    CHECK(calculate(0L, -9223372036854775807L, '+') == -9223372036854775807L);
  }

  SUBCASE("Subtraction") {
    CHECK(calculate(5, 3, '-') == 2);
    CHECK(calculate(0, 0, '-') == 0);
    CHECK(calculate(-5, 10, '-') == -15);
    CHECK(calculate(1000000000, 2000000000, '-') == -1000000000);
    // CHECK(calculate(-9223372036854775807L, 1L, '-') == -9223372036854775808L); // 64-bit overflow.
    // CHECK(calculate(9223372036854775807L, -1L, '-') == 9223372036854775808L);  // 64-bit overflow.
    CHECK(calculate(0L, 9223372036854775807L, '-') == -9223372036854775807L);
    CHECK(calculate(-9223372036854775807L, -9223372036854775807L, '-') == 0L);
    // CHECK(calculate(9223372036854775807L, -9223372036854775807L, '-') == 18446744073709551614L); // 64-bit overflow.
    CHECK(calculate(0L, -9223372036854775807L, '-') == 9223372036854775807L);
  }

  SUBCASE("Multiplication") {
    CHECK(calculate(2, 3, '*') == 6);
    CHECK(calculate(0, 5, '*') == 0);
    CHECK(calculate(-5, -2, '*') == 10);
    CHECK(calculate(1000000000L, 2000000000L, '*') == 2000000000000000000L);
    CHECK(calculate(-9223372036854775807L, 1L, '*') == -9223372036854775807L);
    CHECK(calculate(9223372036854775807L, -1L, '*') == -9223372036854775807L);
    CHECK(calculate(0L, 9223372036854775807L, '*') == 0L);
    // CHECK(calculate(-9223372036854775807L, -9223372036854775807L, '*') == 85070591730234615847396907784232501249L); // 64-bit overflow.
    // CHECK(calculate(9223372036854775807L, -9223372036854775807L, '*') == -85070591730234615847396907784232501249L); // 64-bit overflow.
    CHECK(calculate(0L, -9223372036854775807L, '*') == 0L);
  }

  SUBCASE("Division") {
    CHECK(calculate(10, 2, '/') == 5);
    CHECK(calculate(0, 5, '/') == 0);
    CHECK(calculate(-10, 2, '/') == -5);
    CHECK(calculate(1000000000, 2000000000, '/') == 0);
    CHECK(calculate(-9223372036854775807L, 1L, '/') == -9223372036854775807L);
    CHECK(calculate(9223372036854775807L, -1L, '/') == -9223372036854775807L);
    CHECK(calculate(0L, 9223372036854775807L, '/') == 0L);
    CHECK(calculate(-9223372036854775807L, -9223372036854775807L, '/') == 1L);
    CHECK(calculate(9223372036854775807L, -9223372036854775807L, '/') == -1L);
    CHECK(calculate(0L, -9223372036854775807L, '/') == 0L);
  }

  SUBCASE("Modulo") {
    CHECK(calculate(10, 3, '%') == 1);
    CHECK(calculate(0, 5, '%') == 0);
    CHECK(calculate(-10, 3, '%') == -1);
    CHECK(calculate(1000000000, 2000000000, '%') == 1000000000);
    CHECK(calculate(-9223372036854775807L, 1L, '%') == 0L);
    CHECK(calculate(9223372036854775807L, -1L, '%') == 0L);
    CHECK(calculate(0L, 9223372036854775807L, '%') == 0L);
    CHECK(calculate(-9223372036854775807L, -9223372036854775807L, '%') == 0L);
    CHECK(calculate(9223372036854775807L, -9223372036854775807L, '%') == 0L);
    CHECK(calculate(0L, -9223372036854775807L, '%') == 0L);
  }

  SUBCASE("Unsupported Operator") {
    CHECK_THROWS_AS(USE(calculate(2, 3, '^')), std::invalid_argument);
    CHECK_THROWS_AS(USE(calculate(0, 0, '@')), std::invalid_argument);
    CHECK_THROWS_AS(USE(calculate(-5, 10, '$')), std::invalid_argument);
  }

  SUBCASE("Division by Zero") {
    CHECK_THROWS_AS(USE(calculate(5, 0, '/')), calculation_error);
    CHECK_THROWS_AS(USE(calculate(0, 0, '/')), calculation_error);
    CHECK_THROWS_AS(USE(calculate(-10, 0, '/')), calculation_error);
    CHECK_THROWS_AS(USE(calculate(5, 0, '%')), calculation_error);
    CHECK_THROWS_AS(USE(calculate(0, 0, '%')), calculation_error);
    CHECK_THROWS_AS(USE(calculate(-10, 0, '%')), calculation_error);
  }
}

#endif // ENABLE_DOCTESTS
