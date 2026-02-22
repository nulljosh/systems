"""Minimal text editor skeleton using curses."""
import curses
import sys


class Buffer:
    """Text buffer using a list of lines."""
    def __init__(self, lines=None):
        self.lines = lines or [""]
        self.cursor_x = 0
        self.cursor_y = 0

    @classmethod
    def from_file(cls, path):
        with open(path) as f:
            lines = f.read().splitlines()
        return cls(lines or [""])

    def save(self, path):
        with open(path, "w") as f:
            f.write("\n".join(self.lines) + "\n")

    def insert_char(self, ch):
        line = self.lines[self.cursor_y]
        self.lines[self.cursor_y] = line[:self.cursor_x] + ch + line[self.cursor_x:]
        self.cursor_x += 1

    def delete_char(self):
        if self.cursor_x > 0:
            line = self.lines[self.cursor_y]
            self.lines[self.cursor_y] = line[:self.cursor_x - 1] + line[self.cursor_x:]
            self.cursor_x -= 1


class Editor:
    def __init__(self, stdscr, filepath=None):
        self.stdscr = stdscr
        self.filepath = filepath
        self.buf = Buffer.from_file(filepath) if filepath else Buffer()
        self.scroll_y = 0
        self.running = True

    def draw(self):
        self.stdscr.clear()
        h, w = self.stdscr.getmaxyx()
        for i in range(h - 1):
            line_idx = self.scroll_y + i
            if line_idx < len(self.buf.lines):
                line = self.buf.lines[line_idx][:w - 1]
                self.stdscr.addstr(i, 0, line)
        status = f" {self.filepath or '[new]'} | Ln {self.buf.cursor_y + 1} Col {self.buf.cursor_x + 1} "
        self.stdscr.addstr(h - 1, 0, status[:w - 1], curses.A_REVERSE)
        self.stdscr.move(self.buf.cursor_y - self.scroll_y, self.buf.cursor_x)
        self.stdscr.refresh()

    def handle_key(self, key):
        if key == 17:  # Ctrl-Q
            self.running = False
        elif key == 19:  # Ctrl-S
            if self.filepath:
                self.buf.save(self.filepath)
        elif key == curses.KEY_UP and self.buf.cursor_y > 0:
            self.buf.cursor_y -= 1
        elif key == curses.KEY_DOWN and self.buf.cursor_y < len(self.buf.lines) - 1:
            self.buf.cursor_y += 1
        elif key == curses.KEY_LEFT and self.buf.cursor_x > 0:
            self.buf.cursor_x -= 1
        elif key == curses.KEY_RIGHT:
            self.buf.cursor_x += 1
        elif key in (curses.KEY_BACKSPACE, 127):
            self.buf.delete_char()
        elif 32 <= key < 127:
            self.buf.insert_char(chr(key))

    def run(self):
        curses.curs_set(1)
        while self.running:
            self.draw()
            self.handle_key(self.stdscr.getch())


def main(stdscr):
    filepath = sys.argv[1] if len(sys.argv) > 1 else None
    Editor(stdscr, filepath).run()


if __name__ == "__main__":
    curses.wrapper(main)
