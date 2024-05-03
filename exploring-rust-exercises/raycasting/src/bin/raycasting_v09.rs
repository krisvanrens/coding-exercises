use anyhow::Result;
use crossterm::{
    cursor,
    event::{self, Event, KeyCode},
    execute, queue, style, terminal,
};
use raycasting::{map::Map, player::Player, position::Position};
use std::{
    f32::consts::PI,
    io::{stdout, Write},
    time,
};

const FOV: f32 = PI / 3.0;
const MAX_DEPTH: f32 = 15.0;

/// Wall block corner offset positions.
const OFFSETS: [(u16, u16); 4] = [(0, 0), (0, 1), (1, 0), (1, 1)];

/// Distance-to-wall color (distance range [0.0 .. MAX_DEPTH]).
fn dist_to_wall_color(d: f32) -> style::Color {
    let v = ((MAX_DEPTH - 1.2 * d).clamp(0.0, MAX_DEPTH) * 255.0 / MAX_DEPTH) as u8;
    style::Color::Rgb { r: v, g: v, b: v }
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

                let hit_wall = map.is_wall(Position::new(xx, yy));
                hit = !map.contains(Position::new(xx, yy)) || hit_wall;

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
                if !map.contains(Position::new(x, y)) {
                    queue!(
                        stdout,
                        cursor::MoveTo(x, y),
                        style::SetForegroundColor(style::Color::White)
                    )?;

                    match y {
                        yy if (dist_ceiling..=dist_floor).contains(&yy) => {
                            queue!(
                                stdout,
                                style::SetForegroundColor(wall_color),
                                style::Print(if bound {
                                    "\u{2591}" // Wall block boundary.
                                } else {
                                    "\u{2588}" // Plain wall texture.
                                })
                            )?
                        }
                        yy if yy > dist_floor => queue!(
                            stdout,
                            style::Print(dist_to_floor_texture(
                                1.0 - (y as f32 - height as f32 / 2.0) / (height as f32 / 2.0),
                            ))
                        )?,
                        _ => queue!(stdout, style::Print(" "))?, // Background 'color'.
                    }
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
