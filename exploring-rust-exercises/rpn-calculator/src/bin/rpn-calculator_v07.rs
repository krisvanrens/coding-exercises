use anyhow::{anyhow, Result};
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
    Eoc,
    Invalid(String),
}

/// Calculator operation state.
enum State {
    Operand1,
    Operand2,
    Operator,
    Result,
}

/// Read from an input source and parse it to a token.
fn read_token(source: &mut impl BufRead) -> Result<Token> {
    let mut input = vec![];
    let read_size = source.read_until(b' ', &mut input)?;

    if !input.is_ascii() {
        return Err(anyhow!("input is not valid ASCII data"));
    }

    if read_size <= 1 && (input.is_empty() || input.first() == Some(&10)) {
        return Ok(Token::Eoc);
    }

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

/// Get the result for a binary calculation.
fn calculate(lhs: i128, rhs: i128, o: Operator) -> Result<i128> {
    match o {
        Operator::Add => lhs
            .checked_add(rhs)
            .ok_or_else(|| anyhow!("integer overflow")),
        Operator::Sub => lhs
            .checked_sub(rhs)
            .ok_or_else(|| anyhow!("integer overflow")),
        Operator::Mul => lhs
            .checked_mul(rhs)
            .ok_or_else(|| anyhow!("integer overflow")),
        Operator::Div => match rhs {
            0 => Err(anyhow!("division by zero")),
            _ => Ok(lhs / rhs),
        },
        Operator::Mod => match rhs {
            0 => Err(anyhow!("division by zero")),
            _ => Ok(lhs % rhs),
        },
    }
}

/// Break out of a loop with an error.
macro_rules! desert {
    ($msg:literal) => {
        break Err(anyhow!($msg))
    };
}

fn main() -> Result<()> {
    let mut stdin = io::stdin().lock();

    let mut got_operator = false;
    let mut s = State::Operand1;
    let mut m = Vec::<i128>::with_capacity(2);

    loop {
        let t = read_token(&mut stdin)?;

        match s {
            State::Operand1 => match t {
                Token::Operand(v) => {
                    m.push(v as i128);
                    s = State::Operand2;
                }
                Token::Operator(_) => desert!("expected operand 1, got operator"),
                Token::Eoc => desert!("expected operand 1, got end-of-calculation"),
                Token::Invalid(i) => desert!("expected operand 1, got invalid token '{i}'"),
            },
            State::Operand2 => match t {
                Token::Operand(v) => {
                    m.push(v as i128);
                    s = State::Operator;
                }
                Token::Eoc => {
                    if got_operator {
                        s = State::Result;
                    } else {
                        desert!("expected operand 2, got end-of-calculation");
                    }
                }
                Token::Operator(_) => desert!("expected operand 2, got operator"),
                Token::Invalid(i) => desert!("expected operand 2, got invalid token '{i}'"),
            },
            State::Operator => match t {
                Token::Operator(o) => {
                    if m.len() != 2 {
                        panic!("expected two elements in memory");
                    }

                    let rhs = m.pop().unwrap();
                    let lhs = m.pop().unwrap();
                    m.push(calculate(lhs, rhs, o)?);

                    got_operator = true;
                    s = State::Operand2;
                }
                Token::Operand(_) => desert!("expected operator, got operand"),
                Token::Eoc => desert!("expected operator, got end-of-calculation"),
                Token::Invalid(i) => desert!("expected operator, got invalid token '{i}'"),
            },
            State::Result => {
                if m.len() != 1 {
                    panic!("expected one element in memory");
                }

                println!("{}", m.pop().unwrap());
                break Ok(());
            }
        }
    }
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
    test("", Token::Eoc);
    test("\n", Token::Eoc);

    let mut b = "ŮñĭçøƋɇ".as_bytes();
    assert!(read_token(&mut b).is_err_and(|e| e.to_string() == "input is not valid ASCII data"));
}

#[test]
#[rustfmt::skip]
fn test_calculate() {
    // Note: most of these tests assume operand values of maximum value i64::MAX.

    assert_eq!(calculate(2, 3, Operator::Add).unwrap(), 5);
    assert_eq!(calculate(0, 0, Operator::Add).unwrap(), 0);
    assert_eq!(calculate(-5, 10, Operator::Add).unwrap(), 5);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Add).unwrap(), -9223372036854775807);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Add).unwrap(), 9223372036854775806);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Add).unwrap(), i64::MAX as i128);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Add).unwrap(), -18446744073709551616);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Add).unwrap(), -1);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Add).unwrap(), i64::MIN as i128);

    assert_eq!(calculate(5, 3, Operator::Sub).unwrap(), 2);
    assert_eq!(calculate(0, 0, Operator::Sub).unwrap(), 0);
    assert_eq!(calculate(-5, 10, Operator::Sub).unwrap(), -15);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Sub).unwrap(), -1000000000);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Sub).unwrap(), -9223372036854775809);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Sub).unwrap(), 9223372036854775808);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Sub).unwrap(), -9223372036854775807);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Sub).unwrap(), 0);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Sub).unwrap(), 18446744073709551615);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Sub).unwrap(), 9223372036854775808);

    assert_eq!(calculate(2, 3, Operator::Mul).unwrap(), 6);
    assert_eq!(calculate(0, 5, Operator::Mul).unwrap(), 0);
    assert_eq!(calculate(-5, -2, Operator::Mul).unwrap(), 10);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Mul).unwrap(), 2000000000000000000);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Mul).unwrap(), -9223372036854775808);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Mul).unwrap(), -9223372036854775807);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Mul).unwrap(), 0);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Mul).unwrap(), 85070591730234615865843651857942052864);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Mul).unwrap(), -85070591730234615856620279821087277056);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Mul).unwrap(), 0);

    assert_eq!(calculate(10, 2, Operator::Div).unwrap(), 5);
    assert_eq!(calculate(0, 5, Operator::Div).unwrap(), 0);
    assert_eq!(calculate(-10, 2, Operator::Div).unwrap(), -5);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Div).unwrap(), 0);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Div).unwrap(), -9223372036854775808);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Div).unwrap(), -9223372036854775807);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Div).unwrap(), 0);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Div).unwrap(), 1);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Div).unwrap(), 0);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Div).unwrap(), 0);

    assert_eq!(calculate(10, 3, Operator::Mod).unwrap(), 1);
    assert_eq!(calculate(0, 5, Operator::Mod).unwrap(), 0);
    assert_eq!(calculate(-10, 3, Operator::Mod).unwrap(), -1);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Mod).unwrap(), 1000000000);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Mod).unwrap(), 0);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Mod).unwrap(), 0);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Mod).unwrap(), 0);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Mod).unwrap(), 0);
    assert_eq!(calculate(i64::MAX as i128, i64::MAX as i128, Operator::Mod).unwrap(), 0);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Mod).unwrap(), 0);

    assert_eq!(calculate(i128::MAX, 1, Operator::Add).map_err(|e| e.to_string()), Err("integer overflow".to_owned()));
    assert_eq!(calculate(i128::MIN, -1, Operator::Add).map_err(|e| e.to_string()), Err("integer overflow".to_owned()));
    assert_eq!(calculate(i128::MIN, 1, Operator::Sub).map_err(|e| e.to_string()), Err("integer overflow".to_owned()));
    assert_eq!(calculate(i128::MAX, -1, Operator::Sub).map_err(|e| e.to_string()), Err("integer overflow".to_owned()));
    assert_eq!(calculate(i128::MAX, i128::MAX, Operator::Mul).map_err(|e| e.to_string()), Err("integer overflow".to_owned()));

    assert_eq!(calculate(5, 0, Operator::Div).map_err(|e| e.to_string()), Err("division by zero".to_owned()));
    assert_eq!(calculate(0, 0, Operator::Div).map_err(|e| e.to_string()), Err("division by zero".to_owned()));
    assert_eq!(calculate(-10, 0, Operator::Div).map_err(|e| e.to_string()), Err("division by zero".to_owned()));

    assert_eq!(calculate(5, 0, Operator::Mod).map_err(|e| e.to_string()), Err("division by zero".to_owned()));
    assert_eq!(calculate(0, 0, Operator::Mod).map_err(|e| e.to_string()), Err("division by zero".to_owned()));
    assert_eq!(calculate(-10, 0, Operator::Mod).map_err(|e| e.to_string()), Err("division by zero".to_owned()));
}
