#pragma once

#include <snes.h>

// Sprite tiles (4bpp) + palette
extern const u8 g_spriteTiles4bpp[];
extern const u16 g_spriteTiles4bpp_len;
extern const u16 g_spritePal[];
extern const u16 g_spritePal_len;

// BG2 grid tiles (4bpp), palette (16 colors), and map (32x32)
extern const u8 g_gridTiles4bpp[];
extern const u16 g_gridTiles4bpp_len;
extern const u16 g_gridPal16[];
extern const u16 g_gridPal16_len;
