use anyhow::Result;
use crossterm::{cursor, event, execute, queue, style, terminal};
use std::io::{stdout, Write};

fn main() -> Result<()> {
    terminal::enable_raw_mode()?;

    let mut stdout = stdout();

    execute!(stdout, terminal::EnterAlternateScreen)?;
    execute!(stdout, terminal::Clear(terminal::ClearType::All))?;
    execute!(stdout, cursor::Hide)?;

    let (width, height) = terminal::size()?;
    let msg = format!("Terminal size: {}x{} chars", width, height);

    queue!(
        stdout,
        cursor::MoveTo((width / 2) - (msg.len() as u16 / 2), height / 2,),
        style::Print(msg)
    )?;

    stdout.flush()?;

    event::read()?;

    execute!(stdout, terminal::LeaveAlternateScreen)?;

    Ok(())
}
