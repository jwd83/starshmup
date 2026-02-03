#include "gfx.h"

// Helpers for compile-time tile init (no references to other objects).
#define ROWS8(a, b) (a), (b), (a), (b), (a), (b), (a), (b), (a), (b), (a), (b), (a), (b), (a), (b)
#define ZEROS16 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

// ----------------------------
// Sprite graphics (4bpp)
// ----------------------------
//
// 9 tiles total:
// - Player: 4 tiles, solid color index 1
// - Enemy:  4 tiles, solid color index 2
// - Bullet: 1 tile, solid color index 3
//
// 4bpp tile format: 32 bytes/tile
// bytes  0-15: planes 0/1 interleaved per row (8 rows)
// bytes 16-31: planes 2/3 interleaved per row (8 rows)

const u8 g_spriteTiles4bpp[] = {
    // Player (4 tiles)
    // tile 0-3 (solid color index 1 => plane0=1)
    // tile 0
    ROWS8(0xFF, 0x00), ZEROS16,
    // tile 1
    ROWS8(0xFF, 0x00), ZEROS16,
    // tile 2
    ROWS8(0xFF, 0x00), ZEROS16,
    // tile 3
    ROWS8(0xFF, 0x00), ZEROS16,

    // Enemy (4 tiles) tile 4-7
    // tile 4-7 (solid color index 2 => plane1=1)
    // tile 4
    ROWS8(0x00, 0xFF), ZEROS16,
    // tile 5
    ROWS8(0x00, 0xFF), ZEROS16,
    // tile 6
    ROWS8(0x00, 0xFF), ZEROS16,
    // tile 7
    ROWS8(0x00, 0xFF), ZEROS16,

    // Bullet (1 tile) tile 8
    // color index 3 (plane0=1, plane1=1)
    // tile 8
    ROWS8(0xFF, 0xFF), ZEROS16,
};

const u16 g_spriteTiles4bpp_len = sizeof(g_spriteTiles4bpp);

// Sprite palette (16 colors). CGRAM entries are BGR555 little-endian.
// Color 0 is transparent.
const u16 g_spritePal[] = {
    0x0000, // 0: transparent
    0x03E0, // 1: green
    0x7C1F, // 2: magenta
    0x7FE0, // 3: yellow
    0x7FFF, // 4: white
    0x4210, // 5: gray
    0x001F, // 6: blue
    0x7C00, // 7: red
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
const u16 g_spritePal_len = sizeof(g_spritePal);

// ----------------------------
// Grid BG (4bpp) on BG2
// ----------------------------

const u8 g_gridTiles4bpp[] = {
    // tile 0: minor (color index 1 => plane0)
    0xFF, 0x00, // row 0: full top line
    0x80, 0x00, // row 1: left column
    0x80, 0x00, // row 2
    0x80, 0x00, // row 3
    0x80, 0x00, // row 4
    0x80, 0x00, // row 5
    0x80, 0x00, // row 6
    0x80, 0x00, // row 7
    ZEROS16,     // planes 2/3

    // tile 1: major (color index 2 => plane1)
    0x00, 0xFF, // row 0: full top line
    0x00, 0x80, // row 1: left column
    0x00, 0x80, // row 2
    0x00, 0x80, // row 3
    0x00, 0x80, // row 4
    0x00, 0x80, // row 5
    0x00, 0x80, // row 6
    0x00, 0x80, // row 7
    ZEROS16,     // planes 2/3
};

const u16 g_gridTiles4bpp_len = sizeof(g_gridTiles4bpp);

// BG palette #1 (colors 16-31). Only indices 1 and 2 are used by the grid tiles.
const u16 g_gridPal16[] = {
    0x0000, // 0: black
    0x0010, // 1: dark blue (minor lines)
    0x001F, // 2: bright blue (major lines)
    0x0000, // 3
    0x0000, // 4
    0x0000, // 5
    0x0000, // 6
    0x0000, // 7
    0x0000, // 8
    0x0000, // 9
    0x0000, // 10
    0x0000, // 11
    0x0000, // 12
    0x0000, // 13
    0x0000, // 14
    0x0000, // 15
};

const u16 g_gridPal16_len = sizeof(g_gridPal16);

#undef ROWS8
#undef ZEROS16
