# Starshmup - SNES Space Shoot-em-up

## Project Overview
A space shmup (shoot-em-up) prototype for the Super Nintendo, built with PVSnesLib.

## Build System
- **Toolchain**: PVSnesLib (devkitsnes)
- **Build**: `./build.sh`
- **Clean**: `./build.sh clean`
- **Debug build**: `./build.sh debug`
- **Output**: `starshmup.sfc`
- **PVSnesLib path**: defaults to `./build/pvsneslib` (override with `PVSNESLIB_HOME`)

## Project Structure
```
main.c           # Main game loop, scene management, player/enemy/bullet logic
scenes.h         # Scene enum and shared state (GameStats)
scene_title.c    # Title screen scene
scene_gameover.c # Game over screen scene
gfx.c            # Raw graphics data (4bpp tiles, palettes)
gfx.h            # Graphics asset declarations
data.asm         # Font binary includes for console text (BG1)
pvsneslibfont.pic # Font tiles (4bpp) used by console text
pvsneslibfont.pal # Font palette used by console text
hdr.asm          # SNES ROM header (memory map, vectors, metadata)
Makefile         # Build configuration (includes snes_rules)
build.sh         # Build script (use this instead of make directly)
output/imagegen/ # Generated sprite art (PNGs, not used directly in build)
```

## SNES Hardware Configuration
- **Video Mode**: Mode 1 (BG1/BG2 4bpp, BG3 2bpp)
- **BG1**: Console text overlay (HUD: LEVEL / KILLS)
- **BG2**: Scrolling starfield background
- **Sprites**: Player (16x16), Enemy (16x16), Bullets (8x8)

## VRAM Layout
| Address | Contents |
|---------|----------|
| 0x3000  | BG1 text tiles (console font) |
| 0x4000  | BG2 starfield tiles (BG2_TILE_BASE) |
| 0x5000  | BG2 tilemap (BG2_MAP_BASE) |
| 0x6800  | BG1 text tilemap |
| 0x8000  | Sprite tiles (SPR_TILE_BASE) |

## Sprite Tile Allocation
- Tiles 0-3: Player (16x16 saucer)
- Tiles 4-7: Enemy (16x16 bio-bomb)
- Tile 8: Bullet (8x8 torpedo)

## Sprite Palette Allocation
Each sprite type has its own 16-color palette to avoid conflicts:
| Palette | CGRAM Entry | Sprite Type | Key Colors |
|---------|-------------|-------------|------------|
| 0 | 128-143 | Player | white, gray, cyan, dark |
| 1 | 144-159 | Enemy | green, white, magenta, dark |
| 2 | 160-175 | Bullet | yellow, white, dark |

When adding new sprite types, allocate the next available palette (3-7).

## Key Constants
- Screen: 256x224 pixels
- Player speed: 2 px/frame
- Enemy speed: 1 px/frame (homing)
- Bullet speed: 4 px/frame
- Max bullets: 8 concurrent
- Bullet spawn rate: every 6 frames
- Bullet collision radius: 10 px (approx; axis-aligned check)
- Player collision radius: 12 px (approx; axis-aligned check)
- Level-up: every 10 kills (enemy HP = level)

## Graphics Format
- Tiles: 4bpp, 8x8 pixels, 32 bytes per tile
- Palettes: BGR555 format (native SNES)
- Gameplay graphics are embedded in gfx.c; console font data is included via `data.asm` (`pvsneslibfont.pic` / `pvsneslibfont.pal`)

## Scene System
- **SCENE_TITLE**: Press START to begin gameplay
- **SCENE_GAMEPLAY**: Main game loop (see below)
- **SCENE_GAMEOVER**: Shows final kills/level, press START to return to title

Scene transitions hide all sprites and redraw text as needed.

## Game Loop Order (SCENE_GAMEPLAY)
1. Read controller (D-pad + START)
2. Update player + aim direction
3. Spawn/update bullets
4. Update enemy AI (homing) + collisions (kills/level)
5. Check player-enemy collision (triggers SCENE_GAMEOVER)
6. Update starfield scroll values
7. Write OAM entries for sprites
8. Wait for V-Blank
9. Update HUD text (only when values change)
10. Apply BG2 scroll registers and `oamUpdate()`

## Development Notes
- Use `oamSet()` and `oamSetEx()` for sprite management
- **OAM indices are byte offsets**: sprite 0 = 0, sprite 1 = 4, sprite 2 = 8, etc.
- Use `dmaCopyVram()` and `dmaCopyCGram()` for VRAM/palette transfers
- RNG uses 16-bit Galois LFSR (`rng_next_u16()`)
- Player bounds: 0..240 x, 0..208 y (accounts for 16x16 sprite)
- Pad input is automatic via VBlank ISR (no init needed)

## Testing
Run the output ROM in a SNES emulator (bsnes, snes9x, Mesen-S) or on real hardware via flash cart.
