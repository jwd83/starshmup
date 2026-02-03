# starshmup (SNES)

A space shmup prototype for Super Nintendo, built with PVSnesLib.

- D-pad movement
- Autofire in last-move direction
- Homing enemy
- Collision detection
- Scrolling grid background

## Quick Start (macOS)

1. Download the latest PVSnesLib release for macOS:
   https://github.com/alekmaul/pvsneslib/releases

2. Unzip it into the `build/` folder:
   ```sh
   mkdir -p build
   unzip pvsneslib_*_darwin.zip -d build/
   ```

3. Remove macOS quarantine flags:
   ```sh
   xattr -r -d com.apple.quarantine build/pvsneslib
   ```

4. Build:
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
hdr.asm      # ROM header
Makefile     # Build config
build.sh     # Build script
```

## Technical Details

- Video Mode 1 (BG1/BG2 4bpp, BG3 2bpp)
- 16x16 player/enemy sprites, 8x8 bullets
- Max 8 concurrent bullets
- 16-bit Galois LFSR for RNG
