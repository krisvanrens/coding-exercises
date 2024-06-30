#include <fmt/core.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <exception>
#include <iostream>
#include <iterator>
#include <locale>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <variant>

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

namespace Tokens {

/// Operand token.
struct Operand {
  const std::string value;

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
using Memory = std::stack<long>;

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
                m.push(o.parse());
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
                m.push(o.parse());
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

                const auto rhs = m.top();
                m.pop();
                const auto lhs = m.top();
                m.pop();
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

            const auto result = m.top();
            m.pop();
            std::cout << result << '\n';

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
