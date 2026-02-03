# starshmup (SNES)

Basic shmup prototype for SNES using **pvsneslib**:

- D-pad movement
- Autofire in last-move direction
- One enemy that homes toward you
- Bullet/enemy + player/enemy collisions
- Simple “interstellar graph paper” scrolling background

## Prereqs

Install pvsneslib and its toolchain, then set:

```sh
export PVSNESLIB_HOME="/path/to/pvsneslib"
```

## Build

```sh
make clean
make
```

Output ROM: `starshmup.sfc`

## Run

- Open `starshmup.sfc` in an emulator (bsnes/snes9x), or copy to your flash cart.

## Controls

- D-pad: move (also sets aim direction)
- Autofire: always on (fires in the last non-zero move direction; defaults to up)
