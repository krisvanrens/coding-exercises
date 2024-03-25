# RPN calculator exercise solutions

This set of source files implements possible solutions for the RPN calculator exercise.

As usual with programming exercises, there is no such thing as "the solution" -- many options are possible.
These solutions are mine, and opinionated by my experience, taste and style.
Feel free to divert and disagree.

The goal is to learn from this coding exercise, so just consider my approach to the solutions as one of the many options out there.

## Build requirements

A compiler able to deal with C++20.
You might need a system-installed [{fmt} formatting library](https://github.com/fmtlib/fmt) if your standard library doesn't implement the `<format>` module/header.
You can manually build it, or install `libfmt-dev` for any package-managed Linux environment (this will also install the Cmake module).

For this simple setup I used [Cmake](https://cmake.org/) as a build system.
See build instructions below.
Your IDE might well be able to parse Cmake files to create a project.

For the last step of this exercise, I optionally added Doctest as a test framework dependency.
A custom Cmake module was used to download this library.

## Build instructions

Note: your IDE of choice might be able to parse Cmake files to create a project, in that case you can skip this section.

Create make files:

```sh
mkdir build
cd build
cmake ..
```

Build (still from `build/`):

```sh
make
```

That's it!

### Include tests

For building the last exercise step including tests, we also need to specify an additional command-line argument:

```sh
cmake .. -DENABLE_DOCTESTS
make
```

This will enable the `ENABLE_DOCTESTS` option in the Cmake build and pass the right information to the compiler.

To run the executable with the tests, there are three options:

1. Run only the tests: `./rpn-calculator_vXX --exit`
2. Run only the RPN calculator: `./rpn-calculator_vXX --no-run`
3. Run both the tests and the RPN calculator: `./rpn-calculator_vXX`

To get help regarding the possible command-line options, use the command-line option `--help`.

## How to work with all the code versions?

Your code is probably very different from mine, that's fine.
But this also means you can't just compare your code to mine one-to-one.

Use a diff tool to reveal the difference between each of the versions, and use that difference to check out a possible solution to an exercise step.
Popular diff tools may include `diff`, `meld`, `vimdiff` or perhaps your IDE/editor of choice supports diff views.
I do strongly recommend [`difftastic`](https://github.com/Wilfred/difftastic), which is a diff tool that actually understands language syntax.

For example, to use `vimdiff` to see what's changed between versions 06 and 07:

```sh
vimdiff rpn-calculator_v06.cpp rpn-calculator_v07.cpp
```

## Version descriptions

Each of the versions incrementally build on the previous one.

### Version 00: Application/build setup

This is just a simple application setup to make sure your build environment is set up.

In my solution code later on I will use the `{fmt}` formatting library because many compilers still lack support for `std::format`.
To that end, I used Cmake to get `{fmt}` and link it against the executable.

### Version 01: Reading user input

This version is the previous version including basic reading of user input.

For reading of user input from the command line, we can use `std::cin` from the I/O streams library `<iostream>`.

By default, `std::cin` uses a space character as a separator, and will go end-of-file when the input string ends explicitly.
A return (newline) is ignored.
The input stream ends directly when feeding it Ctrl-D during interactive mode, or when using an empty "here string" from the command line directly.
This means if we want to use `std::cin` in interactive mode, we need Ctrl-D as an explicit 'exit signal', or end the program itself.
A sentence of multiple words is processed word-for-word.

We can test the behavior in the command-line:

**Test 1**: Enter a string in interactive mode:

```sh
$ ./rpn-calculator_v01
Hello there.
Got input: 'Hello'
```

The first word is streamed into the `input` string, then the program ends without processing the rest.

**Test 2**: Enter a string in interactive mode, hit Ctrl-D before ending:

```sh
$ ./rpn-calculator_v01
Hello there.Got input: 'Hello'
```

The first word is streamed into the `input` string, then the program ends without processing the rest.
Because we ended without hitting the return key, there is no newline in the output.

**Test 3**: Hit Ctrl-D without entering any text:

```sh
$ ./rpn-calculator_v01
Got EOF!
```

There is no input, and Ctrl-D caused an EOF immediately.

**Test 4**: Feed an empty "here string" in the command-line:

*Note: This can be tested using a regular Linux command-line shell.*

```sh
$ ./rpn-calculator_v01 <<< ""
Got EOF!
```

There is no input, and the string ended immediately causing an EOF.

**Test 5**: Feed a non-empty "here string" in the command-line:

```sh
$ ./rpn-calculator_v01 <<< "Hello there."
Got input: 'Hello'
```

The first word is streamed into the `input` string, then the program ends without processing the rest.

I hope you now have a feeling for how `std::cin` works.
Input is streamed word-for-word (separated by spaces), and ends at EOF caused by input end (Ctrl-D or here string end).
To process multiple words, we can simply loop over the input reading code.
In later versions we are going to need this to process a calculation term-by-term.
E.g. the calculation `5 4 *` is going to be processed as `5`, `4` and `*` in order.

Whenever using I/O streams, make sure to properly check for errors and flags.
Look at [the page for std::ios_base::iostate on cppreference.com](https://en.cppreference.com/w/cpp/io/ios_base/iostate) for more info.

Notably, the `good()` state is `false` when the input stream is at EOF.
This is something to take into account.

### Version 02: Input classification

This version is the previous version including the ability to distinguish/classify input data.

In order to distinguish between operands (i.e. the input numbers), the operator (i.e. add/subtract/multiply/etc.) and invalid input (everything else) we need to classify the input string.
Whenever your application accepts foreign user input, it is absolutely essential that this data is checked/sanitized before further processing.
User input abuse is one of the major attack vectors for security exploits.

To limit the scope of this exercise, we are first going to focus on integer operand support only.
It is quite doable to support any kind of input number (floating-point, integer, scientific notation, etc.), but for now we'll stick to positive/negative integer support only.

The following subsections explain how the solution code classifies input strings.

#### Operands: positive integers

Positive integer operands have a nice characteristic: all characters in the input string are digits 0..9.
Using the [standard library algorithm `all_of`](https://en.cppreference.com/w/cpp/algorithm/ranges/all_any_none_of) we can test if all characters adhere to a simple "is it a digit" test.
By using the `std::ranges::all_of` version (the so-called constrained algorithms), we only have to supply the range (i.e. the input string) and the test predicate.

Because the test predicate is used more than once, I created a local function called `is_digit`.
Note that I explicitly use a naming scheme for these predicates that intuitively fits to what the rest of the standard library offers.

#### Operands: negative integers

The difference between this operand category and the positive numbers is the fact that negative numbers start with a minus sign.
Other than that, the conditions are the same as the previous operand category.o

Two remarks:

- I used [C++20's `std::string::starts_with()`](https://en.cppreference.com/w/cpp/string/basic_string/starts_with) to check for the minus sign. No need to hassle with character indexes.
- To start the `is_digit` test for the rest of the string, I offset the range using [`std::next()` from `<iterator>`](https://en.cppreference.com/w/cpp/iterator/next). Because we need a modified range here, we cannot use the constrained algorithm version of `all_of`.

#### Operators

Operators are simply tested against a set of known-supported operators, defined as a `constexpr std::string` globally.
Global variables are bad generally speaking, but that is mostly the case when using global *mutable* state.
Global immutable definitions are fine most of the time.

To test for a valid operator input, the [standard library algorithm `any_of`](https://en.cppreference.com/w/cpp/algorithm/ranges/all_any_none_of) is used on the operator set in combination with an input length test.

All other input is simply discarded as "invalid".

### Version 03: Input tokenization

This version is the previous version including an abstraction around the input data called `Token`.

In order for the rest of the code to be able to work with the input data, we can create an abstraction over it.
We will first take the `enum class`-approach to see where it goes.

There are two things we need to store:

- The token type (operand/operator/invalid),
- The token value (a `std::string` for now).

This version uses a `struct` with a `type` field that is an `enum class` (the discriminant of the token), and a field `value` carrying the value.

Already in this simple version you will notice that this setup with the `struct` is not ideal.
In C++, `enum class` is simply a sort of "glorified integer with a name", but not a real enumerator as we'd like to use it.
The cleanest way to match the `type` field is to use a `switch` statement which feels clunky at best.
Also, the fact that the value of the token is always there (even for invalid tokens) and the fact that it is always a `std::string` (even for operators) is not ideal either.

In a later solution version we will use `std::variant` for proper discriminant union support.

Furthermore: I moved the input reading and token creation into its own function returning an `optional<Token>`.
An optional, because when input reading EOF is reached, the caller should be able know this.

Errors are processed as exceptions.

### Version 04: Token parsing

This version is the previous version including parsing of the input token value.

Before we can implement the actual calculations, we must parse the token string value to an integer value in the case of an operand.
In C++ this can be done in several ways, but by far the most performant and robust way is to implement this using [C++17's `std::from_chars`](https://en.cppreference.com/w/cpp/utility/from_chars).

Number parsing is a surprisingly hard problem (try to implement it robustly by hand!), and the interface of `std::from_chars` takes some getting used to.
As usual, errors are dealt with using exceptions.
Because of how the data structure for the `Token` is setup, we must take into account that `parse` may be called on an empty or invalid token.

### Version 05: Calculation support

This version is the previous version including calculation support for a single calculation with two operands.

Let's bring the calculator to life!
In this step, a function called `calculate` is added that, depending on the provided operator, calculates the result of an operation with two operands.

Next to support for `enum`/`enum class`/any integers, a `switch` statement can also be used for 8-bit character values.
The operator token string value essentially is a character value, so we can use it here to "parse" the operator.

The main function is now adapted to support a single full calculator with two operands and one operator.

From this version onward, the test script `rpn-calculator_test.sh` can be used to verify compliance.
It can be run as follows (e.g. from the `build/` directory):

```sh
$ ../rpn-calculator_test.sh ./rpn-calculator_v05
```

Many tests will fail at first, because the required functionality is not implemented yet.
As your implementation progresses with the upcoming versions, more tests should pass.

### Version 06: Using a stack for calculation memory

This version is the previous version including using the right memory data structure for calculation: a stack.

To set up the RPN calculator for further extension later on, we must use the right memory data structure: a stack.
Each operand is pushed on the stack, and when an operator is processed, the operands are popped off the stack to use in the calculation.
Using this will make it possible to implement the RPN calculator as a state machine in the following steps.

The C++ standard library has a container adapter called [`std::stack`](https://en.cppreference.com/w/cpp/container/stack).

### Version 07: Main state machine setup

This version is the previous version including the machinery to deal with new calculations repeatedly.

We define an `enum class` called `State` to deal with the state machine `operand1 --> operand2 --> operator --> ..back..`.

Again, this implementation is a "first step".
We'll see that there are alternative ways to implement state machines in C++ that map much better to the problem space.
In a later version we will use `std::variant` to implement a state machine.

We will first focus on moving the existing machinery into the state machine and deal with corner cases in the next version.

### Version 08: Filling in the state machine flow

This version is the previous version including the machinery to deal with each possible situation from each state correctly.

For the previous version, control flow for the happy path is fine, but any deviation will lead to weird behavior and errors.
For example, when entering an operator instead of the second operand, the program will stop with a vague error.
Also, there is no clean way to stop the program without generating an error.
To fix this, we will cover all possible situations in all possible states.

### Version 09: Better error handling

This version is the previous version including a more consistent way of error handling.

To improve the error handling, we define a custom error type, called `calculation_error` that can be used as an exception.
The idea behind this is that we catch this error at an intermediate level, reset the calculator state and move on instead of stopping the program.
Any other (fatal) exception will stop the program.

Some additional tests dealing with error handling from the test script should now pass as well.

### Version 10: A real discriminant union

This version is the previous version including the use of `std::variant` to group tokens rather than using an `enum`.

Remember back in version 03 I mentioned that the `struct` with a `enum class` field was a poor representation of the token type.
We are going to take an alternative approach for this now.

The most drawbacks of the previous implementation of `Token` are:

- A `struct` is a product type. What I mean by this: every field of the `struct` can have a certain amount of states, so the total amount of states the `struct` as a whole can have is the product of all its field states. And very often, many of those possible states are invalid (in fact, by far most of them!). For example: the `type` could be `Operand`, and the `value` could be `rjk29`; a nonsensical state. In other words: the data model for `Token` does not cleanly map to the problem space (i.e. a valid token). A token should either be:
  - An operand with number value,
  - An operator with an operator character value,
  - An invalid token without a value at all.
- We can call `parse` on any token, even an invalid one.

What we will do is create a separate *type* for each of the token types.
This token-specific-type will then dictate what fields it has, or has not, and what functions it has.

But we cannot use a `switch` statement on types.
Therefore we will change the `Token` to be a `std::variant` over the token types.
The [`std::visit` function](https://en.cppreference.com/w/cpp/utility/variant/visit) will "visit" a token instantiation and call a function depending on the type the token currently holds (i.e. the token type).
This is slightly complex machinery, that can be used more easily using the `overload` helper defined at the top.
The token type implementations are wrapped in a namespace to keep their names grouped and relatable.

As you can see now, only the `Operand` token has a `parse` function (meaning we cannot call `parse` on a non-operand).
The `Operator` token type only has a character value to store its operand.
An `Invalid` type token has no fields at all.
All these changes mean that the invalid token state space is drastically limited down to what it should be: (almost) only valid states.

The `overload` helper uses variadic template parameters to be able to deal with all variant states at once, using function overloads.
It's OK if you don't fully understand how this works at first, variadic template parameters and parameter packs are advanced topics that might need some time to land.

### Version 11: Support for longer calculations

This version is the previous version including support for calculations with more than two operands and a single operator.

To support sequenced calculations, we need to simply modify the state machine.
Rather than immediately print the calculation result, we put the result back on the stack for further calculations.
However, now we need a way to know when we should actually print the calculation result.
Also, the "end-of-calculation" must be communicated to the state machine.

So here's what will change:

- We add a state `Result` to print the calculation result,
- We add a token type `Eoc` (End-of-calculation) to propagate the EOC,
- We add a check to test for a longer calculations and the behavior for a EOC token in the `Operand2` state.

We can now remove the `std::optional` return value for `read_token` (because we have a dedicated token type for "no token"/"EOC") and change it to `Token`.

Although now the RPN calculator supports long calculations, it is not able to deal with repeated calculations in the interactive mode.
Trade offs, trade offs.
Oh well.

### Version 12: A better approach to state machines

This version is the previous version including the use of `std::variant` for safer state machines.

Similar to the changes we made to the `Token` enumerator earlier, we can change the `State` enumerator into a `std::variant`.
All the safety benefits of custom types for states as explained in an earlier step hold.
At this point in the development of the RPN calculator the benefit may not be as big for `State` as for `Token`, but in my opinion it is still worth it.

For example: we could now move the `calculate` function to the `States::Operator` state, as it is only relevant to be called in this state.
However, there is something weird about about a class `States::Operator` without associated data and a single non-static function.
All things come together in the main state machine mechanics, and up until this point none of the data is associated directly to any state.

By the way, as you will see in the table way below, the use of `std::variant` as an abstraction doesn't influence the amount of machine instructions in the executable.
Even though a `switch` statement is such a low-level element, the compiler is able to see through and optimize most if not all of the `std::variant` abstractions.

### Version 13: Improving robustness

This version is the previous version including solving several corner cases that may lead to errors.

If you have run the test script, you will have seen that there are situations where calculations fail.

In this version we fix division by zero errors.
The result of integer division by zero, which is what we have to deal with here, cannot be represented.
Depending on the hardware platform at hand, it most often leads to some kind of hardware exception (e.g. X86 handles it this way).
For C++, division by zero is treated as "undefined behavior", a "hole in the standard spec" if you will.
This simply means you have to deal with it yourself, and shouldn't rely on the compiler/hardware/etc. to back you up.

There are more classes of errors that can be fixed, some more easily than others.
Can you think of some?
How, for example, can we manage integer over- or underflow?

### Version 14: Making the calculation generic

This version is the previous version including making the calculation function generic.

Take into account the possibility that generic type `T` is a floating-point type, for which the normal `%` modulo operator is not supported.

This step to making the code generic is one of the building blocks needed for floating-point number support, as implemented in a later version.

### Version 15: Making parsing generic

This version is the previous version including making the parsing function generic.

If we want our calculator to support other number types, we must also genericize the operand parsing function.
This is done by making `parse` a function template with a type template parameter.

Generic parsing does complicate matters slightly though.
For example, what happens if we encounter the value string `"12.3"` and we have to parse it to `long`?
Or, what if we encounter the value string `"123456789012345678901234567890"` (which exceeds the value that can be represented with 64 bits)?
That why my implementation has some safe guards built in.
For the cross-parsing errors I chose to use `if constexpr` in the primary function template, but this can also be done using specialization (which would lead to more duplicate code though).

Also, to limit the scope of the implementation, I defined a concept called `signed_arithmetic` to require the template argument to be any signed arithmetic type.

### Version 16: Using a custom stack implementation

This version is the previous version including the use of a custom stack container implementation.

Up until this verison, we used the `std::stack` container adapter from the C++ standard library.
However, in our limited-scope RPN, calculator, we only need a stack of a fixed maximum size of two elements.
This can probably be implemented much more efficiently than the fully generic `std::stack` based on a `std::deque`.

You may recall from the course that we implemented our own stack type `Stack` that takes a type template parameter and a maximum size non-type template parameter.
We can create a specialization of `Stack` that is specialized on a maximum stack size of two elements and make it very simple and efficient.

If you look at the table in the section at the bottom showing the amount of machine instructions for each version of the calculator, you'll see that we safe about 350 machine instructions with this change.
Again, this doesn't mean the program is any more resource-efficient or faster, it just means there is less machine code in the compiler output.
However, in this case we can probably safely assume we really win both in performance *and* efficiency at the same time.

### Version 17: Adding unit tests

This version is the previous version including inline unit tests for the free functions.

In C++ unit tests are usually implemented using a test framework offering assertions and other pleasantries like fixtures/logging/etc.
Aside from some minimal functions like `assert` there is no extensive test-support in the C++ language itself.
Test frameworks like Catch2, GoogleTest and Doctest are opinionated, so pick the one you like best.

In this version I added unit tests for all the functions except for `main` using the Doctest single-header test framework.
See the build instructions above on how to include the tests in the build and run them.

### Version 18: Support for floating-point calculations

This version is version 16 including support for single-precision floating-point calculations.

Because we made both the operand parsing function `parse` and the calculation function `calculate` generic, we can easily apply this change.
In principle, we just need to change the template arguments to the `parse` calls, and the template type argument of the stack.

But this is not all.
I included this version because it brings to light some of the design decisions we made earlier on and how they impact a change like this.
The `read_token` function for example, makes all kinds of assumptions on the representation of a token for validation.
This does not hold anymore for floating-point input which can look quite different from (signed) integer input, like scientific notation `3.433e2`.
This simply means we must shift the whole invalidation responsibility of an operand to the `parse` function.
This invalidation *needed to be* in `read_token` for integer-only input, because we always parsed any input string as a `long double` anyway.
At first you'll find that `std::from_chars` is surprisingly relaxed at accepting input.
We need some additional checking to validate the whole operand.

## Number of machine instructions per executable

The following table shows the number of machine instructions per executable version:

| Executable | Number of instructions |
|:----------:|:-----------------------|
| rpn-calculator_v00 | 122 |
| rpn-calculator_v01 | 244 |
| rpn-calculator_v02 | 403 |
| rpn-calculator_v03 | 751 |
| rpn-calculator_v04 | 937 |
| rpn-calculator_v05 | 1048 |
| rpn-calculator_v06 | 1390 |
| rpn-calculator_v07 | 1607 |
| rpn-calculator_v08 | 1430 |
| rpn-calculator_v09 | 1594 |
| rpn-calculator_v10 | 1814 |
| rpn-calculator_v11 | 1869 |
| rpn-calculator_v12 | 1877 |
| rpn-calculator_v13 | 1907 |
| rpn-calculator_v14 | 1907 |
| rpn-calculator_v15 | 2059 |
| rpn-calculator_v16 | 1730 |
| rpn-calculator_v17 | 1740 |
| rpn-calculator_v18 | 1428 |

The compiler used was GCC-12.3 for an AMD Ryzen 7 PRO 4750 running Ubuntu Linux 22.04.3.

Please note that the number of instructions is nothing more than an indication of how "efficient" a compiler is (dependent on target machine etc.).
It's in no way an indication of the resource-efficiency of the program itself.

The data can be generated from the Linux command-line with:

```sh
for exe in rpn-calculator_v??; do echo "${exe}, $(objdump -d ${exe} | grep -c '^[[:space:]]\+[0-9a-f]\+:')"; done
```

## Some ideas for extensions

Here are some ideas to further extend the exercise:

- Add extra operators.
- Safety: what happens if we dump non-ASCII data into the calculator? E.g. by using: `cat /dev/urandom | ./rpn_calculator_v17`
- Create an abstraction around the calculator as a whole, and add unit tests.
- Use `tl::expected` (an implementation of C++23 `std::expected`) for error handling.
- Word-for-word vs. line-at-a-time reading token reading.
- More useful error messages (e.g. curly lines under token).
- Alternative number bases.
- Big numbers handling, larger than 64 bits (e.g. through GMP).
- Add a network interface for input.

Also, if you liked this exercise, and need more challenging, build yourself a [Forth language](https://en.wikipedia.org/wiki/Forth_(programming_language)) interpreter!

