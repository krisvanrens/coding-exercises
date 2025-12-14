use anyhow::{anyhow, Result};
use rpn_calculator::{calculate, token_reader::IterTokens, Token};
use std::io;

/// Calculator operation state.
enum State {
    /// Expecting the first operand of a binary calculation.
    Operand1,
    /// Expecting the second operand of a binary calculation.
    Operand2,
    /// Expecting the operator of a binary calculation.
    Operator,
}

/// Break out of a loop with an error.
macro_rules! desert {
    ($msg:literal) => {
        return Err(anyhow!($msg))
    };
}

fn main() -> Result<()> {
    let stdin = io::stdin().lock();

    let mut got_operator = false;
    let mut s = State::Operand1;
    let mut m = Vec::<i128>::with_capacity(2);

    for t in stdin.iter_tokens() {
        let t = t?;
        match s {
            State::Operand1 => match t {
                Token::Operand(v) => {
                    m.push(i128::from(v));
                    s = State::Operand2;
                }
                Token::Operator(_) => desert!("expected operand 1, got operator"),
                Token::Eoc => unreachable!("Eoc should never be yielded by iterator"),
                Token::Invalid(i) => desert!("expected operand 1, got invalid token '{i}'"),
            },
            State::Operand2 => match t {
                Token::Operand(v) => {
                    m.push(i128::from(v));
                    s = State::Operator;
                }
                Token::Eoc => unreachable!("Eoc should never be yielded by iterator"),
                Token::Operator(_) => desert!("expected operand 2, got operator"),
                Token::Invalid(i) => desert!("expected operand 2, got invalid token '{i}'"),
            },
            State::Operator => match t {
                Token::Operator(o) => {
                    assert!(m.len() == 2, "expected two elements in memory");

                    let rhs = m.pop().unwrap();
                    let lhs = m.pop().unwrap();
                    m.push(calculate(lhs, rhs, o)?);

                    got_operator = true;
                    s = State::Operand2;
                }
                Token::Operand(_) => desert!("expected operator, got operand"),
                Token::Eoc => unreachable!("Eoc should never be yielded by iterator"),
                Token::Invalid(i) => desert!("expected operator, got invalid token '{i}'"),
            },
        }
    }

    // Iterator ended - check if we have a valid final state.
    match s {
        State::Operand1 => desert!("expected operand 1, got end-of-calculation"),
        State::Operand2 => {
            if got_operator {
                assert!(m.len() == 1, "expected one element in memory");

                println!("{}", m.pop().unwrap());

                Ok(())
            } else {
                desert!("expected operand 2, got end-of-calculation")
            }
        }
        State::Operator => desert!("expected operator, got end-of-calculation"),
    }
}
