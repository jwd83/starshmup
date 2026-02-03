#include "gfx.h"

// Tile data helpers
#define ROWS8(a, b) (a), (b), (a), (b), (a), (b), (a), (b), (a), (b), (a), (b), (a), (b), (a), (b)
#define ZEROS16 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

// Sprite tiles (4bpp, 32 bytes/tile)
// Layout: Player (tiles 0-3), Enemy (tiles 4-7), Bullet (tile 8)
const u8 g_spriteTiles4bpp[] = {
    // Player tiles 0-3 (color index 1)
    ROWS8(0xFF, 0x00), ZEROS16,
    ROWS8(0xFF, 0x00), ZEROS16,
    ROWS8(0xFF, 0x00), ZEROS16,
    ROWS8(0xFF, 0x00), ZEROS16,
    // Enemy tiles 4-7 (color index 2)
    ROWS8(0x00, 0xFF), ZEROS16,
    ROWS8(0x00, 0xFF), ZEROS16,
    ROWS8(0x00, 0xFF), ZEROS16,
    ROWS8(0x00, 0xFF), ZEROS16,
    // Bullet tile 8 (color index 3)
    ROWS8(0xFF, 0xFF), ZEROS16,
};

const u16 g_spriteTiles4bpp_len = sizeof(g_spriteTiles4bpp);

// Sprite palette (BGR555): transparent, green, magenta, yellow, white, gray, blue, red
const u16 g_spritePal[] = {
    0x0000, 0x03E0, 0x7C1F, 0x7FE0, 0x7FFF, 0x4210, 0x001F, 0x7C00,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
const u16 g_spritePal_len = sizeof(g_spritePal);

// Grid tiles for BG2 (4bpp): minor lines (tile 0) and major lines (tile 1)
const u8 g_gridTiles4bpp[] = {
    // Tile 0: minor grid (color index 1)
    0xFF, 0x00,  0x80, 0x00,  0x80, 0x00,  0x80, 0x00,
    0x80, 0x00,  0x80, 0x00,  0x80, 0x00,  0x80, 0x00,
    ZEROS16,
    // Tile 1: major grid (color index 2)
    0x00, 0xFF,  0x00, 0x80,  0x00, 0x80,  0x00, 0x80,
    0x00, 0x80,  0x00, 0x80,  0x00, 0x80,  0x00, 0x80,
    ZEROS16,
};
const u16 g_gridTiles4bpp_len = sizeof(g_gridTiles4bpp);

// BG palette #1: black, dark blue (minor), bright blue (major)
const u16 g_gridPal16[] = {
    0x0000, 0x0010, 0x001F, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};
const u16 g_gridPal16_len = sizeof(g_gridPal16);

#undef ROWS8
#undef ZEROS16
