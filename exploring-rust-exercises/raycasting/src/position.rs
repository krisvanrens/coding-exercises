use std::ops::Add;

/// 2D position abstraction.
#[derive(Clone)]
pub struct Position<T> {
    pub x: T,
    pub y: T,
}

impl<T: Copy + Add<Output = T>> Position<T> {
    /// Create a new position from coordinate values.
    pub fn new(x: T, y: T) -> Self {
        Self { x, y }
    }

    /// Create a new delta-offset position from an existing one.
    pub fn adjusted(&self, dx: T, dy: T) -> Self {
        Position::<T>::new(self.x + dx, self.y + dy)
    }
}

impl From<Position<f32>> for Position<u16> {
    fn from(pos: Position<f32>) -> Position<u16> {
        Position::new(pos.x as u16, pos.y as u16)
    }
}
