use anyhow::Result;
use crossterm::{cursor, event, execute, queue, style, terminal};
use std::{
    f32::consts::PI,
    io::{stdout, Write},
};

/// The level map definition.
const MAP: &str = "####################\
                   #   ##             #\
                   #   ##             #\
                   #                  #\
                   #         ##########\
                   #                  #\
                   ######             #\
                   #    #      ###    #\
                   #    #      ###    #\
                   #                  #\
                   #                  #\
                   ####################";

const MAP_WIDTH: u16 = 20;
const MAP_HEIGHT: u16 = 12;

const FOV: f32 = PI / 3.0;
const MAX_DEPTH: f32 = 15.0;

fn main() -> Result<()> {
    terminal::enable_raw_mode()?;

    let mut stdout = stdout();

    execute!(stdout, terminal::LeaveAlternateScreen)?;
    execute!(stdout, terminal::Clear(terminal::ClearType::All))?;
    execute!(stdout, cursor::Hide)?;

    let (width, height) = terminal::size()?;

    let player_x: f32 = 7.0;
    let player_y: f32 = 1.0;
    let player_angle: f32 = 0.0;

    for x in 0..width {
        let ray_angle = player_angle - (FOV / 2.0) + (x as f32 * FOV) / width as f32;
        let norm_x = ray_angle.sin();
        let norm_y = ray_angle.cos();

        let mut dist_wall: f32 = 0.0;
        let mut hit = false;
        while !hit && dist_wall < MAX_DEPTH {
            dist_wall += 0.1;

            let xx = (player_x + norm_x * dist_wall) as u16;
            let yy = (player_y + norm_y * dist_wall) as u16;

            hit = xx >= MAP_WIDTH
                || yy >= MAP_HEIGHT
                || MAP.as_bytes()[(MAP_WIDTH * yy + xx) as usize] == b'#';
        }

        let dist_ceiling = ((height as f32 / 2.0) - (height as f32 / dist_wall)).round() as u16;
        let dist_floor = height - dist_ceiling;

        let wall_color = match dist_wall {
            x if (0.0..=(MAX_DEPTH * 0.25)).contains(&x) => "\u{2588}",
            x if ((MAX_DEPTH * 0.25)..=(MAX_DEPTH * 0.5)).contains(&x) => "\u{2593}",
            x if ((MAX_DEPTH * 0.5)..=(MAX_DEPTH * 0.75)).contains(&x) => "\u{2592}",
            x if ((MAX_DEPTH * 0.75)..=MAX_DEPTH).contains(&x) => "\u{2591}",
            _ => " ",
        };

        for y in 0..height {
            queue!(
                stdout,
                cursor::MoveTo(x, y),
                style::Print(match y {
                    yy if (dist_ceiling..=dist_floor).contains(&yy) => wall_color,
                    yy if yy > dist_floor => {
                        match 1.0 - (y as f32 - height as f32 / 2.0) / (height as f32 / 2.0) {
                            d if (0.0..=0.25).contains(&d) => "#",
                            d if (0.25..=0.5).contains(&d) => "x",
                            d if (0.5..=0.75).contains(&d) => "-",
                            d if (0.75..=0.9).contains(&d) => ".",
                            _ => " ",
                        }
                    }
                    _ => " ",
                })
            )?;
        }
    }

    stdout.flush()?;

    event::read()?;

    execute!(stdout, terminal::LeaveAlternateScreen)?;
    execute!(stdout, cursor::Show)?;

    Ok(())
}
