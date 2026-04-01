from __future__ import annotations

import struct
import zlib
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parent
ASSET_DIR = ROOT / "assets-cg"

FONT = {
    "A": ["01110", "10001", "10001", "11111", "10001", "10001", "10001"],
    "C": ["01111", "10000", "10000", "10000", "10000", "10000", "01111"],
    "G": ["01110", "10001", "10000", "10111", "10001", "10001", "01110"],
    "I": ["11111", "00100", "00100", "00100", "00100", "00100", "11111"],
    "M": ["10001", "11011", "10101", "10101", "10001", "10001", "10001"],
    "O": ["01110", "10001", "10001", "10001", "10001", "10001", "01110"],
    "P": ["11110", "10001", "10001", "11110", "10000", "10000", "10000"],
    "R": ["11110", "10001", "10001", "11110", "10100", "10010", "10001"],
    "S": ["01111", "10000", "10000", "01110", "00001", "00001", "11110"],
    "T": ["11111", "00100", "00100", "00100", "00100", "00100", "00100"],
    "U": ["10001", "10001", "10001", "10001", "10001", "10001", "01110"],
    "Y": ["10001", "10001", "01010", "00100", "00100", "00100", "00100"],
    " ": ["00000", "00000", "00000", "00000", "00000", "00000", "00000"],
}


def chunk(tag: bytes, data: bytes) -> bytes:
    crc = zlib.crc32(tag + data) & 0xFFFFFFFF
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)


def write_png(path: Path, width: int, height: int, pixels: list[tuple[int, int, int]]) -> None:
    rows = []
    for y in range(height):
        start = y * width
        row = bytearray([0])
        for r, g, b in pixels[start : start + width]:
            row.extend([r, g, b])
        rows.append(bytes(row))
    raw = b"".join(rows)
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    path.write_bytes(data)


def draw_rect(pixels: list[tuple[int, int, int]], width: int, x: int, y: int, w: int, h: int, color: tuple[int, int, int]) -> None:
    for py in range(y, y + h):
        for px in range(x, x + w):
            pixels[py * width + px] = color


def draw_line(pixels: list[tuple[int, int, int]], width: int, x0: int, y0: int, x1: int, y1: int, color: tuple[int, int, int]) -> None:
    dx = abs(x1 - x0)
    dy = -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        pixels[y0 * width + x0] = color
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def draw_text(pixels: list[tuple[int, int, int]], width: int, x: int, y: int, text: str, scale: int, color: tuple[int, int, int]) -> None:
    cursor = x
    for char in text:
        glyph = FONT.get(char.upper(), FONT[" "])
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit != "1":
                    continue
                draw_rect(pixels, width, cursor + gx * scale, y + gy * scale, scale, scale, color)
        cursor += (5 + 1) * scale


def make_icon(bg: tuple[int, int, int], accent: tuple[int, int, int], text: tuple[int, int, int], path: Path) -> None:
    width, height = 92, 64
    pixels = [bg for _ in range(width * height)]
    draw_rect(pixels, width, 0, 0, width, 14, accent)
    draw_rect(pixels, width, 8, 18, 76, 36, (248, 250, 252))
    draw_line(pixels, width, 16, 46, 16, 26, accent)
    draw_line(pixels, width, 16, 46, 68, 46, accent)
    draw_line(pixels, width, 20, 42, 34, 36, accent)
    draw_line(pixels, width, 34, 36, 48, 40, accent)
    draw_line(pixels, width, 48, 40, 60, 28, accent)
    draw_text(pixels, width, 18, 4, "AP", 1, text)
    draw_text(pixels, width, 22, 50, "MICRO", 1, accent)
    draw_text(pixels, width, 54, 4, "STUDY", 1, text)
    write_png(path, width, height, pixels)


def main() -> None:
    ASSET_DIR.mkdir(parents=True, exist_ok=True)
    make_icon((225, 235, 243), (21, 96, 147), (255, 255, 255), ASSET_DIR / "icon-uns.png")
    make_icon((17, 58, 92), (248, 192, 68), (255, 255, 255), ASSET_DIR / "icon-sel.png")
    print(f"Wrote {ASSET_DIR / 'icon-uns.png'}")
    print(f"Wrote {ASSET_DIR / 'icon-sel.png'}")


if __name__ == "__main__":
    main()
