use anyhow::Result;
use std::io::{self, BufRead};

#[derive(Debug, PartialEq)]
enum Operator {
    Add,
    Sub,
    Mul,
    Div,
    Mod,
}

/// Calculation token (as parsed from the user input).
#[derive(Debug, PartialEq)]
enum Token {
    Operand(i64),
    Operator(Operator),
    Invalid(String),
}

/// Read from an input source and parse it to a token.
fn read_token(source: &mut impl BufRead) -> Result<Token> {
    let mut input = vec![];
    source.read_until(b' ', &mut input)?;

    let parse_operator = |c: char| -> Option<Operator> {
        match c {
            '+' => Some(Operator::Add),
            '-' => Some(Operator::Sub),
            '*' => Some(Operator::Mul),
            '/' => Some(Operator::Div),
            '%' => Some(Operator::Mod),
            _ => None,
        }
    };

    let input = String::from_utf8_lossy(&input).trim().to_string();

    if input.len() == 1 {
        if let Some(o) = parse_operator(input.chars().next().unwrap()) {
            return Ok(Token::Operator(o));
        }
    }

    match input.parse::<i64>() {
        Ok(v) => Ok(Token::Operand(v)),
        Err(_) => Ok(Token::Invalid(input)),
    }
}

fn main() -> Result<()> {
    let mut stdin = io::stdin().lock();

    while let Ok(t) = read_token(&mut stdin) {
        if dbg!(t) == Token::Invalid("".to_owned()) {
            break; // Stop at the first empty string.
        }
    }

    Ok(())
}

#[test]
fn test_read_token() {
    let test = |input: &str, expected| {
        let mut b = input.as_bytes();
        assert_eq!(read_token(&mut b).unwrap(), expected);
    };

    test("123", Token::Operand(123));
    test("-456", Token::Operand(-456));
    test("+", Token::Operator(Operator::Add));
    test("^", Token::Invalid("^".to_owned()));
    test("abc", Token::Invalid("abc".to_owned()));
    test("", Token::Invalid("".to_owned()));
    test("\n", Token::Invalid("".to_owned()));
}
