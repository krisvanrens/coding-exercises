use crate::position::Position;
use std::{
    f32::consts::{PI, TAU},
    fmt,
};

/// Player movement speed.
const MOVE_SPEED: f32 = 0.1;

/// Player rotation speed in [radians].
const ROTATION_SPEED: f32 = 0.1;

/// Player state.
pub struct Player {
    pub pos: Position<f32>,
    pub angle: f32,
}

impl Player {
    /// Create a new player with a given position and orientation angle.
    #[must_use]
    pub fn new(pos: Position<f32>, angle: f32) -> Self {
        Self { pos, angle }
    }

    /// Move up the player position if the predicate call checks out (called with new position).
    pub fn move_up_if(&mut self, pred: impl Fn(Position<f32>) -> bool) {
        self.move_if(pred, MOVE_SPEED);
    }

    /// Move down the player position if the predicate call checks out (called with new position).
    pub fn move_down_if(&mut self, pred: impl Fn(Position<f32>) -> bool) {
        self.move_if(pred, -MOVE_SPEED);
    }

    /// Move the player position with a specific speed (and direction).
    fn move_if(&mut self, pred: impl Fn(Position<f32>) -> bool, speed: f32) {
        let dx = speed * self.angle.sin();
        let dy = speed * self.angle.cos();

        // Try full diagonal movement.
        let new_pos = self.pos.adjusted(dx, dy);
        if pred(new_pos) {
            self.pos = new_pos;
            return;
        }

        // Try sliding along x-axis if diagonal blocked.
        let new_pos = self.pos.adjusted(dx, 0.0);
        if pred(new_pos) {
            self.pos = new_pos;
            return;
        }

        // Try sliding along y-axis if x-axis blocked.
        let new_pos = self.pos.adjusted(0.0, dy);
        if pred(new_pos) {
            self.pos = new_pos;
        }
    }

    /// Turn the player counter-clockwise by a bit.
    pub fn turn_ccw(&mut self) {
        self.angle = (self.angle - ROTATION_SPEED).rem_euclid(TAU);
    }

    /// Turn the player clockwise by a bit.
    pub fn turn_cw(&mut self) {
        self.angle = (self.angle + ROTATION_SPEED).rem_euclid(TAU);
    }
}

impl fmt::Display for Player {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        const D: f32 = PI / 8.0;
        write!(
            f,
            "{}",
            match self.angle {
                a if a > (TAU - D) || a <= D => "\u{21D3}", // Downwards arrow.
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
