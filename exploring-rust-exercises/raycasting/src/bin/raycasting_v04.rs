use anyhow::Result;
use crossterm::{
    cursor,
    event::{self, Event, KeyCode},
    execute, queue, style, terminal,
};
use std::{
    f32::consts::PI,
    io::{stdout, Write},
    ops::Add,
};

const PI2: f32 = PI * 2.0;

const FOV: f32 = PI / 3.0;
const MAX_DEPTH: f32 = 15.0;

#[derive(Clone, Copy)]
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

    fn contains(&self, pos: impl Into<Position<u16>>) -> bool {
        let pos: Position<u16> = pos.into();
        (pos.x as usize) < self.width && (pos.y as usize) < self.height
    }

    fn is_wall(&self, pos: impl Into<Position<u16>>) -> bool {
        let pos: Position<u16> = pos.into();
        self.layout.as_bytes()[self.stride * pos.y as usize + pos.x as usize] == b'#'
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

    fn move_up_if(&mut self, pred: impl FnOnce(Position<f32>) -> bool) {
        let new_pos = self
            .pos
            .adjusted(0.1 * self.angle.sin(), 0.1 * self.angle.cos());
        if pred(new_pos) {
            self.pos = new_pos;
        }
    }

    fn move_down_if(&mut self, pred: impl FnOnce(Position<f32>) -> bool) {
        let new_pos = self
            .pos
            .adjusted(-0.1 * self.angle.sin(), -0.1 * self.angle.cos());
        if pred(new_pos) {
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
        for x in 0..width {
            let ray_angle = p.angle - (FOV / 2.0) + (x as f32 * FOV) / width as f32;
            let norm_x = ray_angle.sin();
            let norm_y = ray_angle.cos();

            let mut dist_wall: f32 = 0.0;
            let mut hit = false;
            while !hit && dist_wall < MAX_DEPTH {
                dist_wall += 0.1;

                let xx = (p.pos.x + norm_x * dist_wall) as u16;
                let yy = (p.pos.y + norm_y * dist_wall) as u16;

                hit = !map.contains(Position::new(xx, yy)) || map.is_wall(Position::new(xx, yy));
            }

            let dist_ceiling = ((height as f32 / 2.0) - (height as f32 / dist_wall)).round() as u16;
            let dist_floor = height - dist_ceiling;
            let wall_color = dist_to_wall_color(dist_wall);

            for y in 0..height {
                queue!(
                    stdout,
                    cursor::MoveTo(x, y),
                    style::Print(match y {
                        yy if (dist_ceiling..=dist_floor).contains(&yy) => wall_color,
                        yy if yy > dist_floor => dist_to_floor_texture(
                            1.0 - (y as f32 - height as f32 / 2.0) / (height as f32 / 2.0),
                        ),
                        _ => " ", // Background 'color'.
                    })
                )?;
            }
        }

        stdout.flush()?;

        if let Event::Key(e) = event::read()? {
            match e.code {
                KeyCode::Char('w') => p.move_up_if(|p| !map.is_wall(p)),
                KeyCode::Char('s') => p.move_down_if(|p| !map.is_wall(p)),
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
