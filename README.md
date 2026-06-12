# Debris

Debris is a fully playable, multi-level, Tetris-inspired falling-block game for
PRG32. It uses the `riscv-prg32/DeviceDemo` cartridge structure as a template,
but replaces the demo pages with a complete game loop.

## Features

- Player-controlled movement, rotation, hard drop, soft drop, and hold.
- Three-piece next queue.
- Line clears, score, level progression, pause, game over, and restart.
- Procedural geometric graphics and generated sound effects, with no
  third-party copyrighted art or audio.

## Controls

- `LEFT` / `RIGHT`: move
- `A`: rotate
- `UP`: hard drop
- `DOWN`: soft drop
- `B`: hold or swap piece
- `SELECT`: start, pause, resume, or restart

## Build

Keep the PRG32 firmware repository next to this repository, or set
`PRG32_REPO`. Builds are portable ABI-table cartridges by default:

```sh
export PRG32_REPO=/path/to/PRG32
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh
```

The output cartridge is written to `dist/debris-esp32c6.prg32`.

## Documentation

- [Build and publish guide](docs/build-and-publish.md)
- [Step-by-step teaching tutorial](docs/teaching-tutorial.md)
