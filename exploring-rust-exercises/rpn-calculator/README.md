# RPN calculator exercise solutions

This set of source files implements possible solutions for the RPN calculator exercise.

As usual with programming exercises, there is no such thing as "the solution" -- many options are possible.
These solutions are mine, and opinionated by my experience, taste and style.
Feel free to divert and disagree.

The goal is to learn from this coding exercise, so just consider my approach to the solutions as one of the many options out there.

## How to work with all the code versions?

Your code is probably very different from mine, that's fine.
But this also means you can't just compare your code to mine one-to-one.

Use a diff tool to reveal the difference between each of the versions, and use that difference to check out a possible solution to an exercise step.
Popular diff tools may include `diff`, `meld`, `vimdiff` or perhaps your IDE/editor of choice supports diff views.
I do strongly recommend [`difftastic`](https://github.com/Wilfred/difftastic), which is a diff tool that actually understands language syntax.

For example, to use `vimdiff` to see what's changed between versions 06 and 07:

```sh
vimdiff rpn-calculator_v04.rs rpn-calculator_v05.rs
```

## Running the external test script

To run the test script that tests the executable as a whole, first build the project, then run the tests as follows (e.g. for testing version 04):

```sh
./rpn-calculator_test.sh target/debug/rpn-calculator_v04
```

If you want to run the cargo tests, run:

```sh
cargo test
```

## Version descriptions

Each of the versions incrementally build on the previous one.

### Version 00: Application setup

Create a new project with an executable using:

```sh
cargo new
```

You can now start working in `src/main.rs`.

#### Building and running

Build the executable using:

```sh
cargo build
```

Run it with:

```sh
cargo run
```

In a POSIX-like shell, you can use standard input for testing fixed inputs:

```sh
echo "4 55 *" | cargo run
```

#### Naming conventions

This project containing the solution code features multiple executables, or "binary targets" as they are called.
If you want multiple executables in a single project, this can be achieved in several ways:

- Use `src/main.rs` as a binary target,
- Use one or more source files in `src/bin/` that will be binary targets,
- Use custom `[[bin]]` tables in the `Cargo.toml` configuration.

Read more about this in [The Cargo Book](https://doc.rust-lang.org/cargo/reference/cargo-targets.html#binaries).

I chose the naming conventions-based solution simply because it takes the least amount of work.
Hey, I was told good developers are lazy developers!

### Version 01: Reading from standard input

This version is the previous version including reading from standard input.

Reading from a processes' standard input in Rust can be done with objects implementing the [`Stdin` trait](https://doc.rust-lang.org/std/io/struct.Stdin.html).
A handle to standard input is a shared reference to the global buffer.
In Rust, any unprotected access to global resources is prohibited, so we need to lock the standard input handle.
This can be done explicitly, with `lock` (returning a lock handle to access the resource), or implicitly by using `lines` or `read_line`.

I will use [`read_line`](https://doc.rust-lang.org/std/io/struct.Stdin.html#method.read_line) in the example, this method returns a result indicating the number of bytes read, or an I/O error.
To "just" use the `read_line` call, we must "unpack" this result value to get to the return value.
For this pragmatic example step, I've used `expect` to let the `Ok` value pass, and add a custom panic message upon an `Err` result.

Method `read_line` reads until the next newline character, which is included in the result string.
By using [`trim`](https://doc.rust-lang.org/std/primitive.str.html#method.trim) on the result string, we can remove all leading and trailing "whitespace" characters (which includes newline in this case).

### Version 02: Input data tokenization and parsing

This version is the previous version including basic input data tokenization.

We'll add the code to distinguish the token categories:

- Operators (one of the following characters: `+-*/%`),
- Operands (any signed integer),
- Invalid (anything else).

To formalize these tokens, we can use an enumerator in Rust.
This is a *true* enumerator in the sense that it is a discriminant union, taking one of several variants.
Each of the enumerator variants can carry values, for example a signed integer for an operand.
We will use this `Token` type for the classification code.

Building on the previous version, we read a line from standard input as a whole, but we want to parse each word separately.
We can use [the `std::str::split` method](https://doc.rust-lang.org/std/str/struct.Split.html) to split the input line at each white space character.
The result of `split` is an iterator that we can traverse / modify / collect / etc.

The way I set up the business logic of the solution code for this step is by transforming the split string words into tokens, then finally collect a vector of tokens.
The transformation method that converts a string to a `Token` is called `classify`, a local function.
This local function `classify` is called by `map` and thus maps each word to a token enumerator variant, including its value.

The next step is the classification code itself.

First we check for the relevant tokens operator and operand, then consider everything else invalid input.
We pass a string slice to `classify`, which then first checks if a single-character string slice matches one of the operator characters.
This implementation is just a pragmatic first attempt, we may want to change this later.
Accessing the first character of a string may seem very complicated, but think about the (very, very) nice fact that strings in Rust are protected UTF-8 strings, possibly containing multi-byte Unicode characters.
Just indexing a string as if it were an ASCII string would be unsafe as we might just get a single byte from a multi-byte *grapheme*.

If the operator check fails, we continue to try and parse the string slice as a signed 64-bit integer.
The [`parse` method](https://doc.rust-lang.org/std/primitive.str.html#method.parse) is a generic method taking a type argument indicating the parse target type.
It is invoked using the `::<>` part, called the ["TurboFish" syntax](https://www.reddit.com/r/rust/comments/3fimgp/comment/ctozkd0/).
Note how dealing with a result is very nice using the `if let` syntax.

Lastly, if the operand parse failed as well, we will leave the `classify` function with an `Invalid` token variant.
Note that we can omit `return` and the statement ending semicolon to leave the function with a value.

### Version 03: Method for tokenization / parsing

This version is the previous version, but now with input data reading, tokenization, parsing and error handling + tests in a separate method.

There are some subtle changes w.r.t. the previous version, highlighted in the following subsections.

#### The method API

First we will define a separate method called `read_token` that will classify the string data to a token and parse the value if applicable (`Operand` and `Operator`).
Classification and parsing is an inherently fallible mechanism, so we need some way to propagate errors out of the method call.
You may think: "but we have the `Invalid` token for that right?"
Yes, that's true, but the `Invalid` token is meant for "calculator-level" errors, not fatal system errors.
Suppose for some external reason the system memory becomes corrupted, and operand value parsing fails, is the `Invalid` token still the best way to deal with this?
No, in that case we want a fatal error to propagate to the caller of the method, which means we should use a `Result<Token, Error>` for the method return value.
We will discuss the error type in the next subsection.

Then we are left with the method argument; how are we going to ingest the string data?
There are different choices to make here, all with their pros and cons.
For the solution code, I chose to offload as much of the token processing as possible to `read_token`, without assuming the nature of the input source.
What I mean with this last bit is: I don't want to assume standard input as the input source in `read_token`, the source should be universal up to a degree.
There are two reasons for this:

- Choosing to take a universal source makes the method code more reusable (for other sources),
- We can plug in test sources...for testing :-)

The abstraction for the string data source I used is the [`BufReader` trait](https://doc.rust-lang.org/std/io/struct.BufReader.html).
By taking the source as a `impl BufReader`, we indicate simply that the source we pass as a caller must implement the `BufReader` trait.
We can also make the method generic over `B` and require the `B: BufReader` trait bound (i.e. `fn read_token<B: BufReader>(source: &mut B) ...`), but the `impl` notation is much more readable.
Even though the `impl BufReader` notation is syntactic sugar for the named generic parameter, there is a difference between the two notations (read about it [here](https://doc.rust-lang.org/reference/types/impl-trait.html)).

We take the source as a `&mut` to be able to read and modify it.

#### Error handling

In the previous section we discussed the argumentation for using a `Result<T, E>` method return type.
Now, what to choose for `E`, there error type here?

For a simple application like this exercise (up until now at least), it makes little sense to define our own custom error type.
It would require us to define this error type, and possible various conversion into other error types required for error propagation.
Type `E` is a dependent type of `Result<T, E>`, so for each result propagation, we need a conversion step (depending on the `Result` types used at each level.

In cases like this, where we don't have strong arguments to use a custom error type, we can use the types from a crate like `anyhow`, the *defacto* standard in this area.
You can add `anyhow` as a dependency by invoking `cargo add anyhow` from the command line.
Crate `anyhow` provides a result type `Result<T>` that abstracts away the error type (it will use `anyhow::Result` for this).
It also features all the required conversion from and to other result types, so basically it enables us to use `Result`, without having to deal with error types.
Nice!

#### Classification and parsing

In contrast to the previous version, this version reads the input data stream word-for-word, rather than a line at a time.
This changes the classification and parsing mechanics, the `read_token` function reads until the first white space.

Note that the [`BufRead::read_until`](https://doc.rust-lang.org/std/io/trait.BufRead.html#method.read_until) method returns a `Result`, so we can use the question mark operator to have errors propagate to the caller of `read_token`.

Because the input is a byte array, we must build a string from it first.
Then we can implement parsing of operators and operands.

#### The result of `main`

In Rust, the `main` method can have one of two prototypes:

- `fn main()`, i.e. a function without a return value (unit `()` return type),
- `fn main() -> Result<T, E>`, i.e. a function returning a `Result`.

The second form is very convenient when calling functions that return `Result` from `main`, as we can just use the question mark operator to call them.
Any error will simply propagate to `main`.

If `main` exits returns an `Err`, the exit value of the program will be `1`, or `0` upon success.
The POSIX concept of program exit codes can be controlled using [`std::process::exit`](https://doc.rust-lang.org/std/process/fn.exit.html).

#### Tests

Writing unit tests for functions is very easy in Rust.
Just mark a function with the attribute `#[test]`, and invoke `cargo test` from the command line (or use the editor/IDE to invoke the test).

Because we used the `impl BufRead` requirement for `read_token`, we can easily plug in a test source for data.
We can see in [the documentation](https://doc.rust-lang.org/std/io/trait.BufRead.html#implementors) that `BufRead` is implemented for byte slices, so we'll use that in the tests.

### Version 04: Calculation support

This version is the previous version, with simple calculation support.

The `calculate` method in the solution code takes the two operands and a operator, and simply returns the result of the calculation.
To be overflow-safe (sort of) we use signed 128-bit integers for the primitive types.
We will deal with the right way to handle calculation errors/overflow in a future step.

Again we add tests to validate and protect our implementation for breaking changes.

### Version 05: Operand/result memory

This version is the previous version, including a memory that can be used as a stack.

The most logical data structure to use as a stack here is `Vec`, the standard vector type in Rust.

### Version 06: State machine setup

This version is the previous version, including the state machine to process longer calculations.

You will see that after completing this step, most of the calculation steps from the test script will pass.

This is quite a large step again, I'll highlight each of the notable changes in the following subsections.

#### The state machine

The state machine itself is pretty straight-forward and needs no extra explanation.
We represent the states using another enumerator `State`.
In the `main` method we simply loop over the state transitions, and exit the loop upon an error or if we are done (i.e. when the `Result` state is reached).

The loop body comprises of a `match` expression over the current state value mostly.
Each of the state variants then again form a `match` expression over the current token.

To be able to properly end the state machine, we add another token type for "end-of-calculation", `Eoc`.
In state `Operand2`, upon receiving an `Eoc` token, we must detect if we are here for the first time (i.e. `got_operator` is `false`), or if we are processing a longer calculation that now ends (i.e. `got_operator` is `true`).

#### Error handling

Error handling is done using `Result` processing.
For example, if we receive an operator after without having any operands, we exit the loop using `break Err(..)`.
This may seem odd, why are we breaking with a value in the first place, and why are we not returning here?

Many things in Rust are expressions, perhaps more than you're used to, especially if you come from C or C++.
But this can be used to your advantage, as it allows for more expressive flexibility.
In this case, the `loop` is an expression too, and the last thing in the `main` method.
This means the result of the `loop` expression is the result of the `main` method.
That's why we can `break` with a value.

#### The `desert!` macro

To make leaving the state machine with an error slightly more nicer, I decided to introduce a simple macro.
This is a macro called `desert!`, defined using `macro_rules!` syntax.
It expects a literal called `msg`, and will henceforth replace each call to `desert!` with `break Err(anyhow!(..))`.

### Version 07: Improving robustness

This version is the previous version, including robustness fixes to improve handling of corner cases.

What happens if you feed non-ASCII characters to the previous version, or `cat /dev/urandom` into your program?
Is it resilient to this?

What happens if you divide or modulo by zero?
Or, what happens if you enter the value of `i64::MAX` and multiply with itself three times?
To have this work you'd need 64+64+64 bits for representation; this will overflow.

Or, what happens if you `echo ""` into your program (i.e. empty input), does it deal with it correctly?

In this step we try to find corner-cases of operation, and deal with them properly.
In most cases, this is matter of dealing with them explicitly using `if` or `match` expressions.
But to deal with overflow, we can benefit from the `checked_..` methods implemented for number primitive types.
We'll have to convert the return type of the `calculate` method to `Result<i128>` and let the error propagate nicely.

This version should pass all the tests from the test script.
Nice!

### Version 08: Reorganizing and documentation

This version is the previous version, where we organized the single source file into a library + executable, and added documentation.

We simply move the types (except `State`) and methods (except `main`) to a file called `lib.rs` in the crate `src/` directory.
To be able to access the moved types/methods from the executable code, we muse add `use rpn_calculator::*;` (to simply import everything).
That is not all though, we must also add `pub` where appropriate.

Documentation is added to all types and methods.
The documentation for the `read_token` method also features a bit of example code, when running `cargo test` from the command-line, this code is run/tested as well, very nice!
Build the documentation using `cargo doc` from the command-line.

Note that bits of code can be excluded from example code by using this notation:

```rs
/// ```
/// # Lines starting with '#' will be ignored.
/// # This one too.
/// let x = 42; // This will be in the rendered documentation.
/// ```
```

This can be handy to exclude setup code in the documentation example.

### Version 09: Iterator adaptor for token reading

This version is the previous version, with an iterator abstraction over types that implement the `Read` trait.

As a last experiment, we will create a custom iterator adapter over the standard input stream to directly iterate over tokens while reading from standard input.
In essence, this is a simple task, but we need to be careful to have errors propagate correctly.

To make a type iterable, we must implement the `Iterator` trait for that type.
So, in essence we'd want something like `impl<T: Read> Iterator for BufReader<T> { ... }`, but that's not possible.
To avoid ambiguity, Rust defines the so-called 'orphan rules' to dictate that a trait may only implemented if either the type or the trait is local (read about it [here](https://doc.rust-lang.org/reference/items/implementations.html#trait-implementation-coherence)).

But there is another issue: we need the buffer state stored somewhere, so we need an intermediate type anyway.
In the solution code this is the `TokenReader` type, a local type for which we can implement the `Iterator` trait.

Given the above, we could have called it a day.
Create a custom type that stores the `BufReader` state, implement the `Iterator` trait for it and done.
When using it, create a `TokenReader` from the source that you want to iterate over, e.g. by implementing `fn new(..)` for `TokenReader`.

We can make it slightly more ergonomic though, by creating a trait that creates a `TokenReader` when calling a custom iteration function.
This function is called `iter_tokens`, and is implemented for all types that have the `Read` trait implemented.
Now we can directly iterate using `source.iter_tokens()`, nice!

This new iterator adapter plus helpers costs roughly 30 lines of code.
Is it worth it to replace the original `let t = read_token(stdin)?;` call?
It sure looks nicer to do `for t in stdin.iter_tokens() { ... }`..
I'll let you decide, but for me personally it feels a bit like over-engineering, but it's a fun experiment though :-)

## Some ideas for extensions

Here are some ideas to further extend the exercise:

- Add extra operators.
- Change from `i128` to floating-point representation and input.
- Safety: what happens if we dump non-ASCII data into the calculator? E.g. by using: `cat /dev/urandom | target/debug/rpn_calculator_v08`
- Create an abstraction around the calculator as a whole, and add unit tests.
- More useful error messages (e.g. curly lines under token).
- Alternative number bases.
- Really big numbers handling, larger than 128 bits (e.g. through crate `BigInt`).
- Add a network interface for input.

Also, if you liked this exercise, and need more challenging, build yourself a [Forth language](https://en.wikipedia.org/wiki/Forth_(programming_language)) interpreter!
