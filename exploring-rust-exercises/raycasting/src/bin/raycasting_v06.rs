use anyhow::Result;
use crossterm::{
    cursor,
    event::{self, Event, KeyCode},
    execute, queue, style, terminal,
};
use std::{
    f32::consts::PI,
    fmt,
    io::{stdout, Write},
    ops::Add,
    time,
};

const PI2: f32 = PI * 2.0;

const FOV: f32 = PI / 3.0;
const MAX_DEPTH: f32 = 15.0;

/// Wall block corner offset positions.
const OFFSETS: [(u16, u16); 4] = [(0, 0), (0, 1), (1, 0), (1, 1)];

#[derive(Clone)]
struct Position<T> {
    x: T,
    y: T,
}

impl<T: Copy + Add<Output = T>> Position<T> {
    fn new(x: T, y: T) -> Self {
        Self { x, y }
    }

    fn adjusted(&self, dx: T, dy: T) -> Self {
        Position::<T>::new(self.x + dx, self.y + dy)
    }
}

impl From<Position<f32>> for Position<u16> {
    fn from(pos: Position<f32>) -> Position<u16> {
        Position::new(pos.x as u16, pos.y as u16)
    }
}

struct Map {
    layout: String,
    stride: usize,
    width: usize,
    height: usize,
}

impl Map {
    fn new(layout: &str) -> Self {
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

    fn contains(&self, pos: &Position<u16>) -> bool {
        (pos.x as usize) < self.width && (pos.y as usize) < self.height
    }

    fn is_wall(&self, pos: &Position<u16>) -> bool {
        self.layout.as_bytes()[self.stride * pos.y as usize + pos.x as usize] == b'#'
    }
}

impl fmt::Display for Map {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.layout)
    }
}

struct Player {
    pos: Position<f32>,
    angle: f32,
}

impl Player {
    fn new(pos: Position<f32>, angle: f32) -> Self {
        Self { pos, angle }
    }

    fn move_up_if(&mut self, pred: impl FnOnce(&Position<f32>) -> bool) {
        let new_pos = self
            .pos
            .adjusted(0.1 * self.angle.sin(), 0.1 * self.angle.cos());
        if pred(&new_pos) {
            self.pos = new_pos;
        }
    }

    fn move_down_if(&mut self, pred: impl FnOnce(&Position<f32>) -> bool) {
        let new_pos = self
            .pos
            .adjusted(-0.1 * self.angle.sin(), -0.1 * self.angle.cos());
        if pred(&new_pos) {
            self.pos = new_pos;
        }
    }

    fn turn_ccw(&mut self) {
        self.angle = (self.angle - 0.1 + PI2).rem_euclid(PI2);
    }

    fn turn_cw(&mut self) {
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

/// Distance-to-wall color (distance range [0.0 .. MAX_DEPTH]).
fn dist_to_wall_color(d: f32) -> &'static str {
    match d {
        x if (0.0..=(MAX_DEPTH * 0.25)).contains(&x) => "\u{2588}",
        x if ((MAX_DEPTH * 0.25)..=(MAX_DEPTH * 0.5)).contains(&x) => "\u{2593}",
        x if ((MAX_DEPTH * 0.5)..=(MAX_DEPTH * 0.75)).contains(&x) => "\u{2592}",
        x if ((MAX_DEPTH * 0.75)..=MAX_DEPTH).contains(&x) => "\u{2591}",
        _ => " ",
    }
}

/// Distance-to-floor texture (distance range [0.0 .. 1.0]).
fn dist_to_floor_texture(d: f32) -> &'static str {
    match d {
        x if (0.0..=0.25).contains(&x) => "#",
        x if (0.25..=0.5).contains(&x) => "x",
        x if (0.5..=0.75).contains(&x) => "-",
        x if (0.75..=0.9).contains(&x) => ".",
        _ => " ",
    }
}

fn main() -> Result<()> {
    terminal::enable_raw_mode()?;

    let mut stdout = stdout();

    execute!(stdout, terminal::EnterAlternateScreen)?;
    execute!(stdout, terminal::Clear(terminal::ClearType::All))?;
    execute!(stdout, cursor::Hide)?;

    let (width, height) = terminal::size()?;

    let map = Map::new(
        "####################\n\r\
         #   ##             #\n\r\
         #   ##             #\n\r\
         #                  #\n\r\
         #         ##########\n\r\
         #                  #\n\r\
         ######             #\n\r\
         #    #      ###    #\n\r\
         #    #      ###    #\n\r\
         #                  #\n\r\
         #                  #\n\r\
         ####################\n\r",
    );

    let mut p = Player::new(Position::new(7.0, 1.0), 0.0);

    loop {
        let t_start = time::Instant::now();

        for x in 0..width {
            let ray_angle = p.angle - (FOV / 2.0) + (x as f32 * FOV) / width as f32;
            let norm_x = ray_angle.sin();
            let norm_y = ray_angle.cos();

            let mut dist_wall: f32 = 0.0;
            let mut hit = false; // Indicates 'ray hit'.
            let mut bound = false; // Indicates wall block boundary.
            while !hit && dist_wall < MAX_DEPTH {
                dist_wall += 0.1;

                let xx = (p.pos.x + norm_x * dist_wall) as u16;
                let yy = (p.pos.y + norm_y * dist_wall) as u16;

                let hit_wall = map.is_wall(&Position::new(xx, yy));
                hit = !map.contains(&Position::new(xx, yy)) || hit_wall;

                if hit_wall {
                    let mut corners = OFFSETS.map(|(tx, ty)| {
                        let vx = (xx + tx) as f32 - p.pos.x;
                        let vy = (yy + ty) as f32 - p.pos.y;
                        let d = (vx * vx + vy * vy).sqrt();
                        (d, norm_x * vx / d + norm_y * vy / d)
                    });

                    corners.sort_by(|c1, c2| c1.0.partial_cmp(&c2.0).unwrap());

                    bound = (corners[0].1.acos() < 0.01) || (corners[1].1.acos() < 0.01);
                }
            }

            let dist_ceiling = ((height as f32 / 2.0) - (height as f32 / dist_wall)).round() as u16;
            let dist_floor = height - dist_ceiling;
            let wall_color = dist_to_wall_color(dist_wall);

            for y in 0..(height - 1) {
                // Save the last line for frame rate info.
                if !map.contains(&Position::new(x, y)) {
                    queue!(
                        stdout,
                        cursor::MoveTo(x, y),
                        style::Print(match y {
                            yy if (dist_ceiling..=dist_floor).contains(&yy) => {
                                if bound {
                                    "\u{2591}" // Wall block boundary.
                                } else {
                                    wall_color // Plain wall texture.
                                }
                            }
                            yy if yy > dist_floor => dist_to_floor_texture(
                                1.0 - (y as f32 - height as f32 / 2.0) / (height as f32 / 2.0),
                            ),
                            _ => " ", // Background 'color'.
                        })
                    )?;
                }
            }
        }

        queue!(
            stdout,
            cursor::MoveTo(0, 0),
            style::Print(&map),
            cursor::MoveTo(p.pos.x as u16, p.pos.y as u16),
            style::Print(&p)
        )?;

        let t_end = time::Instant::now();

        queue!(
            stdout,
            cursor::MoveTo(0, height - 1),
            style::Print(format!(
                "Frame rate: {:} FPS",
                1e6 as u128 / (t_end - t_start).as_micros()
            ))
        )?;

        stdout.flush()?;

        if let Event::Key(e) = event::read()? {
            match e.code {
                KeyCode::Char('w') => {
                    p.move_up_if(|p| !map.is_wall(&p.clone().into()));
                }
                KeyCode::Char('s') => {
                    p.move_down_if(|p| !map.is_wall(&p.clone().into()));
                }
                KeyCode::Char('a') => p.turn_ccw(),
                KeyCode::Char('d') => p.turn_cw(),
                KeyCode::Char('q') => break,
                _ => (),
            };
        }
    }

    execute!(stdout, terminal::LeaveAlternateScreen)?;
    execute!(stdout, cursor::Show)?;

    Ok(())
}
