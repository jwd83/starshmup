# starshmup (SNES)

A space shmup prototype for Super Nintendo, built with PVSnesLib.

- D-pad movement
- Autofire in last-move direction
- Homing enemy
- Collision detection
- Scrolling starfield background
- HUD: LEVEL + KILLS

## Quick Start (macOS)

1. Download the latest PVSnesLib release for macOS:
   https://github.com/alekmaul/pvsneslib/releases

2. Unzip it into the `build/` folder:
   ```sh
   mkdir -p build
   unzip pvsneslib_*_darwin.zip -d build/
   ```

3. Ensure the extracted folder is at `build/pvsneslib` (rename if the zip used a versioned directory name):
   ```sh
   test -d build/pvsneslib || mv build/pvsneslib_* build/pvsneslib
   ```

4. Remove macOS quarantine flags:
   ```sh
   xattr -r -d com.apple.quarantine build/pvsneslib
   ```

5. Build:
   ```sh
   ./build.sh
   ```

The ROM will be at `starshmup.sfc`.

## Build Commands

```sh
./build.sh        # Build the ROM
./build.sh clean  # Clean build artifacts
./build.sh debug  # Build with debug symbols
```

If PVSnesLib isn’t in `build/pvsneslib`, you can point at it explicitly:

```sh
PVSNESLIB_HOME=/path/to/pvsneslib ./build.sh
```

## Run

Open `starshmup.sfc` in an emulator (bsnes, snes9x, Mesen-S) or flash cart.

## Controls

- **D-pad**: Move (also sets aim direction)
- **Autofire**: Always on, fires in last move direction (defaults to up)

## Project Structure

```
main.c       # Game loop, player/enemy/bullet logic
gfx.c        # Graphics data (tiles, palettes)
gfx.h        # Graphics declarations
data.asm     # Font binary includes for console text (BG1)
pvsneslibfont.pic  # Font tiles (4bpp) used by console text
pvsneslibfont.pal  # Font palette used by console text
hdr.asm      # ROM header
Makefile     # Build config
build.sh     # Build script
```

## Technical Details

- Video Mode 1 (BG1/BG2 4bpp, BG3 2bpp)
- BG1: console text HUD (LEVEL / KILLS)
- BG2: scrolling starfield (hardware scroll)
- 16x16 player/enemy sprites, 8x8 bullets (OAM)
- Max 8 concurrent bullets
- 16-bit Galois LFSR for RNG
- Enemy HP scales with level (level increases every 10 kills)
