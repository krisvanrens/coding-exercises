use crate::position::Position;
use std::fmt;

/// Level map abstraction. Is assumed to contain `#` (octothorpe) symbols for wall block elements.
pub struct Map {
    layout: String,
    stride: usize,
    pub width: usize,
    pub height: usize,
}

impl Map {
    /// Create a new map. Requires the input map layout to be rectangular, ASCII-only and larger
    /// than 3x3 block units. Use `#` (octothorpe) symbols to indicate walls.
    pub fn new(layout: &str) -> Self {
        let stride = layout.find("\n\r").expect("expected a \\n\\r at line end") + 2;
        let width = stride - 2;
        let height = layout.len() / stride;

        assert!(layout.is_ascii()); // Expect ASCII characters only.
        assert_eq!(layout.lines().count(), height + 1); // All lines must end with '\n\r'.
        assert!(width >= 3 && height >= 3); // Expect a minimal level size of 3x3 tiles.
        assert_eq!(layout.len(), stride * height); // Expect a rectangular grid of tiles.

        Self {
            layout: layout.to_owned(),
            stride,
            width,
            height,
        }
    }

    /// Checks if a given position is contained in the map coordinate system.
    pub fn contains(&self, pos: impl Into<Position<u16>>) -> bool {
        let pos: Position<u16> = pos.into();
        (pos.x as usize) < self.width && (pos.y as usize) < self.height
    }

    /// Checks if the given position is a wall block element.
    pub fn is_wall(&self, pos: impl Into<Position<u16>>) -> bool {
        let pos: Position<u16> = pos.into();
        self.layout.as_bytes()[self.stride * pos.y as usize + pos.x as usize] == b'#'
    }
}

impl fmt::Display for Map {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.layout)
    }
}
