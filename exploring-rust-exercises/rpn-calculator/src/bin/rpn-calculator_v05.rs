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

/// Get the result for a binary calculation.
fn calculate(lhs: i128, rhs: i128, o: Operator) -> i128 {
    match o {
        Operator::Add => lhs + rhs,
        Operator::Sub => lhs - rhs,
        Operator::Mul => lhs * rhs,
        Operator::Div => lhs / rhs,
        Operator::Mod => lhs % rhs,
    }
}

fn main() -> Result<()> {
    let mut stdin = io::stdin().lock();

    let t1 = read_token(&mut stdin)?;
    let t2 = read_token(&mut stdin)?;
    let t3 = read_token(&mut stdin)?;

    let mut m = Vec::<i128>::with_capacity(2);

    let Token::Operand(operand1) = t1 else {
        panic!("token 1 is not an operand!");
    };

    m.push(operand1 as i128);

    let Token::Operand(operand2) = t2 else {
        panic!("token 2 is not an operand!");
    };

    m.push(operand2 as i128);

    let Token::Operator(operator) = t3 else {
        panic!("token 3 is not an operator!");
    };

    if m.len() == 2 {
        let rhs = m.pop().unwrap();
        let lhs = m.pop().unwrap();

        println!("Result: {}", calculate(lhs, rhs, operator));
    } else {
        panic!("expected two elements in memory")
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

#[test]
#[rustfmt::skip]
fn test_calculate() {
    // Note: most of these tests assume operand values of maximum value i64::MAX.

    assert_eq!(calculate(2, 3, Operator::Add), 5);
    assert_eq!(calculate(0, 0, Operator::Add), 0);
    assert_eq!(calculate(-5, 10, Operator::Add), 5);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Add), -9223372036854775807);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Add), 9223372036854775806);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Add), i64::MAX as i128);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Add), -18446744073709551616);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Add), -1);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Add), i64::MIN as i128);

    assert_eq!(calculate(5, 3, Operator::Sub), 2);
    assert_eq!(calculate(0, 0, Operator::Sub), 0);
    assert_eq!(calculate(-5, 10, Operator::Sub), -15);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Sub), -1000000000);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Sub), -9223372036854775809);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Sub), 9223372036854775808);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Sub), -9223372036854775807);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Sub), 0);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Sub), 18446744073709551615);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Sub), 9223372036854775808);

    assert_eq!(calculate(2, 3, Operator::Mul), 6);
    assert_eq!(calculate(0, 5, Operator::Mul), 0);
    assert_eq!(calculate(-5, -2, Operator::Mul), 10);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Mul), 2000000000000000000);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Mul), -9223372036854775808);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Mul), -9223372036854775807);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Mul), 0);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Mul), 85070591730234615865843651857942052864);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Mul), -85070591730234615856620279821087277056);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Mul), 0);

    assert_eq!(calculate(10, 2, Operator::Div), 5);
    assert_eq!(calculate(0, 5, Operator::Div), 0);
    assert_eq!(calculate(-10, 2, Operator::Div), -5);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Div), 0);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Div), -9223372036854775808);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Div), -9223372036854775807);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Div), 0);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Div), 1);
    assert_eq!(calculate(i64::MAX as i128, i64::MIN as i128, Operator::Div), 0);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Div), 0);

    assert_eq!(calculate(10, 3, Operator::Mod), 1);
    assert_eq!(calculate(0, 5, Operator::Mod), 0);
    assert_eq!(calculate(-10, 3, Operator::Mod), -1);
    assert_eq!(calculate(1000000000, 2000000000, Operator::Mod), 1000000000);
    assert_eq!(calculate(i64::MIN as i128, 1, Operator::Mod), 0);
    assert_eq!(calculate(i64::MAX as i128, -1, Operator::Mod), 0);
    assert_eq!(calculate(0, i64::MAX as i128, Operator::Mod), 0);
    assert_eq!(calculate(i64::MIN as i128, i64::MIN as i128, Operator::Mod), 0);
    assert_eq!(calculate(i64::MAX as i128, i64::MAX as i128, Operator::Mod), 0);
    assert_eq!(calculate(0, i64::MIN as i128, Operator::Mod), 0);
}
