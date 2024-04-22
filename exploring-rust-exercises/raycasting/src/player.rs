use crate::position::Position;
use std::{f32::consts::PI, fmt};

const PI2: f32 = PI * 2.0;

/// Player state.
pub struct Player {
    pub pos: Position<f32>,
    pub angle: f32,
}

impl Player {
    /// Create a new player with a given position and orientation angle.
    pub fn new(pos: Position<f32>, angle: f32) -> Self {
        Self { pos, angle }
    }

    /// Move up the player position if the predicate call checks out (called with new position).
    pub fn move_up_if(&mut self, pred: impl FnOnce(&Position<f32>) -> bool) {
        let new_pos = self
            .pos
            .adjusted(0.1 * self.angle.sin(), 0.1 * self.angle.cos());
        if pred(&new_pos) {
            self.pos = new_pos;
        }
    }

    /// Move down the player position if the predicate call checks out (called with new position).
    pub fn move_down_if(&mut self, pred: impl FnOnce(&Position<f32>) -> bool) {
        let new_pos = self
            .pos
            .adjusted(-0.1 * self.angle.sin(), -0.1 * self.angle.cos());
        if pred(&new_pos) {
            self.pos = new_pos;
        }
    }

    /// Turn the player counter-clockwise by a bit.
    pub fn turn_ccw(&mut self) {
        self.angle = (self.angle - 0.1 + PI2).rem_euclid(PI2);
    }

    /// Turn the player clockwise by a bit.
    pub fn turn_cw(&mut self) {
        self.angle = (self.angle + 0.1).rem_euclid(PI2);
    }
}

impl fmt::Display for Player {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        const D: f32 = PI / 8.0;
        write!(
            f,
            "{}",
            match self.angle {
                a if a > (PI2 - D) || a <= D => "\u{21D3}", // Downwards arrow.
                a if a > D && a <= (D * 3.0) => "\u{21D8}", // South East arrow.
                a if a > (D * 3.0) && a <= (D * 5.0) => "\u{21D2}", // Rightwards arrow.
                a if a > (D * 5.0) && a <= (PI - D) => "\u{21D7}", // North East arrow.
                a if a > (PI - D) && a <= (PI + D) => "\u{21D1}", // Upwards arrow.
                a if a > (PI + D) && a <= (PI + (D * 3.0)) => "\u{21D6}", // North West arrow.
                a if a > (PI + (D * 3.0)) && a <= (PI + (D * 5.0)) => "\u{21D0}", // Leftwards arrow.
                _ => "\u{21D9}", // South West arrow.
            }
        )
    }
}
