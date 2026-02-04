#pragma once

#include <snes.h>

// Sprite tiles (4bpp)
extern const u8 g_spriteTiles4bpp[];
extern const u16 g_spriteTiles4bpp_len;

// Sprite palettes (one per sprite type to avoid conflicts)
// Player = palette 0, Enemy = palette 1, Bullet = palette 2
extern const u16 g_playerPal[];
extern const u16 g_playerPal_len;
extern const u16 g_enemyPal[];
extern const u16 g_enemyPal_len;
extern const u16 g_bulletPal[];
extern const u16 g_bulletPal_len;

// BG2 grid tiles (4bpp), palette (16 colors), and map (32x32)
extern const u8 g_gridTiles4bpp[];
extern const u16 g_gridTiles4bpp_len;
extern const u16 g_gridPal16[];
extern const u16 g_gridPal16_len;
