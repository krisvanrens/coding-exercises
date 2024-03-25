use std::io;

#[allow(dead_code)] // Silence the compiler.

/// Calculation token (as parsed from the user input).
#[derive(Debug)]
enum Token {
    Operand(i64),
    Operator(char),
    Invalid,
}

const OPERATOR_CHARS: [char; 5] = ['+', '-', '*', '/', '%'];

fn main() {
    let mut input = String::new();
    io::stdin()
        .read_line(&mut input)
        .expect("failed to read input");

    println!("Got input: '{}'", input.trim());

    let classify = |s: &str| -> Token {
        if s.len() == 1 {
            let first = s.chars().next().expect("failed to read first character");
            if OPERATOR_CHARS.contains(&first) {
                return Token::Operator(first);
            }
        }

        if let Ok(v) = s.parse::<i64>() {
            return Token::Operand(v);
        }

        Token::Invalid
    };

    let tokens = input.trim().split(' ').map(classify).collect::<Vec<_>>();

    println!("Got tokens: {tokens:?}");
}
