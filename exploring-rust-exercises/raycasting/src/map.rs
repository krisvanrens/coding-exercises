use crate::position::Position;
use std::fmt;

/// Level map abstraction. Is assumed to contain `#` (octothorpe) symbols for wall block elements.
pub struct Map {
    layout: &'static str,
    stride: u16,
    pub width: u16,
    pub height: u16,
}

impl Map {
    const fn calculate_stride(l: &[u8]) -> usize {
        let mut i = 0;
        loop {
            if i >= l.len() - 1 || (l[i] == b'\n' && l[i + 1] == b'\r') {
                break i + 2;
            }

            i += 1;
        }
    }

    const fn is_ascii(l: &[u8]) -> bool {
        let mut i = 0;
        while i < l.len() {
            if !l[i].is_ascii() {
                return false;
            }

            i += 1;
        }

        true
    }

    /// Create a new map. Requires the input map layout to be rectangular, ASCII-only and larger
    /// than 3x3 block units. Use `#` (octothorpe) symbols to indicate walls.
    pub const fn new(layout: &'static str) -> Self {
        let stride = Self::calculate_stride(layout.as_bytes());
        let width = stride - 2;
        let height = layout.len() / stride;

        assert!(Self::is_ascii(layout.as_bytes())); // Expect ASCII characters only.
        assert!(width >= 3 && height >= 3); // Expect a minimal level size of 3x3 tiles.
        assert!(layout.len() == stride * height); // Expect a rectangular grid of tiles.

        Self {
            layout,
            stride: stride as u16,
            width: width as u16,
            height: height as u16,
        }
    }

    /// Checks if a given position is contained in the map coordinate system.
    pub fn contains(&self, pos: &Position<u16>) -> bool {
        pos.x < self.width && pos.y < self.height
    }

    /// Checks if the given position is a wall block element.
    pub fn is_wall(&self, pos: &Position<u16>) -> bool {
        self.layout.as_bytes()[(self.stride * pos.y + pos.x) as usize] == b'#'
    }
}

impl fmt::Display for Map {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.layout)
    }
}
