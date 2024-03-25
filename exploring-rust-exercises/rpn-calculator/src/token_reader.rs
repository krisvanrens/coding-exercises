use crate::{read_token, Result, Token};
use std::io::{BufReader, Read};

/// Token reader abstraction used for iteration state.
pub struct TokenReader<T: Read> {
    reader: BufReader<T>,
}

impl<T: Read> Iterator for TokenReader<T> {
    type Item = Result<Token>;

    fn next(&mut self) -> Option<Result<Token>> {
        match read_token(&mut self.reader) {
            Ok(t) => Some(Ok(t)),
            Err(e) => Some(Err(e)),
        }
    }
}

/// Helper trait to be able to use token iteration directly on a type that implements `Read`.
pub trait IterTokens {
    type Output;

    fn iter_tokens(self) -> Self::Output;
}

impl<T: Read> IterTokens for T {
    type Output = TokenReader<T>;

    fn iter_tokens(self) -> Self::Output {
        TokenReader {
            reader: BufReader::new(self),
        }
    }
}
