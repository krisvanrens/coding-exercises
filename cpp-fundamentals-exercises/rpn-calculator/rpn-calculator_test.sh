#!/bin/bash -e

if [[ $# != 1 ]] || [[ ! -x $1 ]]
then
  echo >&2 "Usage: $0 <executable_to_test>"
  exit 1
fi

CALCULATOR="$1"

TESTS_PASS=0
TESTS_FAIL=0

log_info() {
  echo -e "\e[1m[INFO]\e[0m: $1"
}

#
# Test the calculator with a given input.
#
# \param $1 Calculation string value.
# \param $2 Expected result string value.
#
test_input() {
  local input="$1"
  local expect="$2"

  local output="$(${CALCULATOR} 2>&1 <<< ${input})"

  echo -en "\e[93m\e[1m[TEST]\e[0m\e[93m: ${input} \e[1m-->\e[0m\e[93m ${expect} ...\e[0m"

  if [[ "${output}" == "${expect}" ]]
  then
    echo -e "\e[1m\e[32mPASS\e[0m"

    TESTS_PASS=$((${TESTS_PASS} + 1))
  else
    echo -e "\e[1m\e[31mFAIL\e[0m"
    echo -e "\e[1m\e[31mExpected: '${expect}', got: '${output}'\e[0m"

    TESTS_FAIL=$((${TESTS_FAIL} + 1))
  fi
}

destructor() {
  log_info ""
  log_info "Test summary: ran $((${TESTS_PASS} + ${TESTS_FAIL})) tests in total, of which ${TESTS_PASS} tests \e[1m\e[32mPASS\e[0med and ${TESTS_FAIL} tests \e[1m\e[31mFAIL\e[0med."
  log_info ""
}

trap destructor EXIT

log_info "Addition tests:"
test_input "5 7 +" "12"
test_input "0 0 +" "0"
test_input "-10 5 +" "-5"
test_input "1000000000 2000000000 +" "3000000000"
test_input "-9223372036854775807 1 +" "-9223372036854775806"
test_input "9223372036854775807 -1 +" "9223372036854775806"
test_input "0 9223372036854775807 +" "9223372036854775807"
test_input "-9223372036854775807 -9223372036854775807 +" "-18446744073709551614"
test_input "9223372036854775807 -9223372036854775807 +" "0"
test_input "0 -9223372036854775807 +" "-9223372036854775807"

log_info "Subtraction tests:"
test_input "10 3 -" "7"
test_input "0 0 -" "0"
test_input "-10 5 -" "-15"
test_input "1000000000 2000000000 -" "-1000000000"
test_input "-9223372036854775807 1 -" "-9223372036854775808"
test_input "9223372036854775807 -1 -" "9223372036854775808"
test_input "0 9223372036854775807 -" "-9223372036854775807"
test_input "-9223372036854775807 -9223372036854775807 -" "0"
test_input "9223372036854775807 -9223372036854775807 -" "18446744073709551614"
test_input "0 -9223372036854775807 -" "9223372036854775807"

log_info "Multiplication tests:"
test_input "6 8 *" "48"
test_input "0 0 *" "0"
test_input "-10 5 *" "-50"
test_input "1000000000 2000000000 *" "2000000000000000000"
test_input "-9223372036854775807 1 *" "-9223372036854775807"
test_input "9223372036854775807 -1 *" "-9223372036854775807"
test_input "0 9223372036854775807 *" "0"
test_input "-9223372036854775807 -9223372036854775807 *" "85070591730234615847396907784232501249"
test_input "9223372036854775807 -9223372036854775807 *" "-85070591730234615847396907784232501249"
test_input "0 -9223372036854775807 *" "0"

log_info "Division tests:"
test_input "15 3 /" "5"
test_input "0 5 /" "0"
test_input "-10 5 /" "-2"
test_input "1000000000 2000000000 /" "0"
test_input "-9223372036854775807 1 /" "-9223372036854775807"
test_input "9223372036854775807 -1 /" "-9223372036854775807"
test_input "0 9223372036854775807 /" "0"
test_input "-9223372036854775807 -9223372036854775807 /" "1"
test_input "9223372036854775807 -9223372036854775807 /" "-1"
test_input "0 -9223372036854775807 /" "0"

log_info "Modulo tests:"
test_input "17 4 %" "1"
test_input "0 5 %" "0"
test_input "-10 5 %" "0"
test_input "1000000000 2000000000 %" "1000000000"
test_input "-9223372036854775807 1 %" "0"
test_input "9223372036854775807 -1 %" "0"
test_input "0 9223372036854775807 %" "0"
test_input "-9223372036854775807 -9223372036854775807 %" "0"
test_input "9223372036854775807 -9223372036854775807 %" "0"
test_input "0 -9223372036854775807 %" "0"

log_info "Longer, mixed calculation tests:"
test_input "4 5 * 5 * 30 - 2 /" "35"
test_input "10 2 / 3 + 4 * 5 -" "27"
test_input "10 2 / 3 + 4 * 5 - 2 /" "13"
test_input "10 2 / 3 + 4 * 5 - 2 / 3 +" "16"
test_input "10 2 / 3 + 4 * 5 - 2 / 3 + 1 +" "17"
test_input "10 2 / 3 + 4 * 5 - 2 / 3 + 1 + 8 *" "136"
test_input "10 2 / 3 + 4 * 5 - 2 / 3 + 1 + 8 * 10 %" "6"
test_input "10 2 / 3 + 4 * 5 - 2 / 3 + 1 + 8 * 10 % -100 +" "-94"

log_info "Division by zero tests:"
test_input "10 0 /" "Error: division by zero"
test_input "0 0 /" "Error: division by zero"
test_input "-10 0 /" "Error: division by zero"
test_input "1000000000 0 /" "Error: division by zero"
test_input "-9223372036854775807 0 /" "Error: division by zero"
test_input "9223372036854775807 0 /" "Error: division by zero"
test_input "0 0 /" "Error: division by zero"
test_input "-9223372036854775807 0 /" "Error: division by zero"
test_input "9223372036854775807 0 /" "Error: division by zero"
test_input "9223372036854775807 -0 /" "Error: division by zero"
test_input "10 0 %" "Error: division by zero"
test_input "0 0 %" "Error: division by zero"
test_input "-10 0 %" "Error: division by zero"
test_input "1000000000 0 %" "Error: division by zero"
test_input "-9223372036854775807 0 %" "Error: division by zero"
test_input "9223372036854775807 0 %" "Error: division by zero"
test_input "0 0 %" "Error: division by zero"
test_input "-9223372036854775807 0 %" "Error: division by zero"
test_input "9223372036854775807 0 %" "Error: division by zero"
test_input "9223372036854775807 -0 %" "Error: division by zero"

log_info "Overflow tests:"
test_input "9223372036854775807 9223372036854775807 * 9223372036854775807 *" "Error: integer overflow"

log_info "Invalid input tests:"
test_input "" "Error: expected operand 1, got end-of-calculation"
test_input "5" "Error: expected operand 2, got end-of-calculation"
test_input "5 +" "Error: expected operand 2, got operator"
test_input "-10 -" "Error: expected operand 2, got operator"
test_input "*" "Error: expected operand 1, got operator"
test_input "10 5 + -" "Error: expected operand 2, got operator"
test_input "10 5 + 3" "Error: expected operator, got end-of-calculation"
test_input "10 5 + 3 2 - *" "Error: expected operator, got operand"
test_input "10 5 + 3 2 - * /" "Error: expected operator, got operand"
test_input "10 5 + 3 2 - * / %" "Error: expected operator, got operand"
test_input "10 5 + 3 2 - * / % 4" "Error: expected operator, got operand"
test_input "10 5 + 3 2 - * / % 4 2" "Error: expected operator, got operand"
