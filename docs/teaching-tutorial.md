# Debris Teaching Tutorial

This tutorial guides first-year computer science and computer engineering
students through the design, implementation, debugging, packaging, and
publication of Debris, a PRG32 falling-block cartridge. It is written as a
sequence of one-hour laboratories. Each laboratory has a concrete product, a
short theoretical focus, and a pragmatic code-deploy-debug cycle.

The exercise is intentionally an imitation game: students first study the
behavior and structure of a complete cartridge, then reproduce it in disciplined
increments. The goal is not to memorize a game recipe. The goal is to learn how
an interactive system is specified, decomposed, implemented, observed, corrected,
and prepared for other people to use.

## Learning Outcomes

At the end of the sequence, students should be able to:

- Explain the PRG32 cartridge lifecycle: initialization, update, drawing, and
  shutdown.
- Represent a two-dimensional game board with arrays and coordinates.
- Encode tetromino shapes, rotations, collision tests, and line clearing.
- Separate model state, input handling, game rules, rendering, audio, and
  metadata.
- Use a repeatable code-deploy-debug cycle on both emulator and device targets.
- Package a cartridge with metadata, icon, screenshot, license information, and
  multi-architecture binaries.
- Publish a release-quality bundle on the Cartridge Store.

## Prerequisites

Students should have:

- Basic C syntax: variables, arrays, functions, structs, enums, and loops.
- A working PRG32 firmware repository next to this repository, or `PRG32_REPO`
  set to the firmware path.
- A terminal, editor, Git, Python 3, and the PRG32 build tools.
- Access to either QEMU or an ESP32-C6 PRG32 device.

Recommended repository layout:

```text
workspace/
  PRG32/
  Debris/
```

## The Code-Deploy-Debug Cycle

Use the same cycle throughout the laboratories. A small, repeatable cycle is the
most important engineering habit in the exercise.

1. **Choose one visible behavior.** Examples: "the title screen appears", "the
   current piece moves left", "a full line disappears".
2. **Make the smallest code change that could produce it.** Avoid adding several
   features in one edit.
3. **Build the cartridge.**

   ```sh
   export PRG32_REPO=/path/to/PRG32
   export PRG32_ARCHITECTURE=qemu
   scripts/build.sh
   ```

4. **Deploy to the current target.** Use QEMU while developing logic and the
   ESP32-C6 board before declaring a laboratory complete.
5. **Observe the result.** Play for at least one minute or until the new behavior
   is exercised.
6. **Debug with evidence.** If the behavior is wrong, identify whether the error
   is in state, input, rules, rendering, audio, or packaging. Add temporary
   counters or simple visual markers when needed, then remove them.
7. **Record the result.** Commit working checkpoints or write a short lab note:
   what changed, what was tested, and what remains uncertain.

For device deployment:

```sh
export PRG32_ARCHITECTURE=esp32c6
scripts/build.sh

python3 "$PRG32_REPO/tools/prg32_game.py" upload \
  dist/debris-esp32c6.prg32 \
  --url http://192.168.4.1
```

Use the board IP shown by the PRG32 setup mode when the device is joined to a
network.

## Laboratory 1: Product Reading and Game Specification

**Estimated duration:** 1 hour

**Objective:** Translate an existing game idea into an implementable
specification.

**Concepts:** Requirements, constraints, controls, target platform, release
criteria.

**Activities:**

1. Run or inspect the finished Debris cartridge.
2. Write a one-page specification containing:
   - Game genre and core loop.
   - Screen size assumptions and board dimensions.
   - Player controls.
   - Win, loss, pause, and restart conditions.
   - Scoring and level progression.
   - Required publication artifacts.
3. Identify what must be deterministic and what may be random.

**Code to add:**

Create `docs/design-notes.md`. This file is not compiled; it is the engineering
contract for the first implementation.

```md
# Debris Design Notes

## Player Experience

- The player controls falling blocks on a 10 by 20 board.
- The objective is to clear horizontal lines and survive as long as possible.
- The game becomes faster as the number of cleared lines increases.

## Controls

- LEFT / RIGHT: move the falling piece.
- A: rotate the falling piece.
- UP: hard drop.
- DOWN: soft drop.
- B: hold or swap one piece.
- SELECT: start, pause, resume, or restart.

## Program Model

The program will keep the board, current piece, score, level, and game state in
memory. Drawing code will read that model and display it; drawing code should not
change the rules of the game.

## Publication Artifacts

The final cartridge needs source code, metadata JSON files, an icon, a
screenshot, a license, and a Cartridge Store bundle.
```

**Expected product:** A concise game specification approved by the instructor.

**Instructor checkpoints:**

- The student distinguishes "what the player sees" from "what the program
  stores".
- The student can explain why publishing needs metadata and not only source
  code.

## Laboratory 2: Cartridge Skeleton

**Estimated duration:** 1 hour

**Objective:** Create a cartridge that builds and displays a title screen.

**Concepts:** PRG32 lifecycle, entry prefix, source organization, first build.

**Implementation steps:**

1. Create or inspect a single source file at `src/debris.c`.
2. Define the four cartridge lifecycle functions:
   - `debris_init`
   - `debris_update`
   - `debris_draw`
   - `debris_shutdown`
3. In `debris_init`, configure display bands and initialize persistent state.
4. In `debris_draw`, clear the screen and draw the title "DEBRIS".
5. Build the cartridge with:

   ```sh
   export PRG32_REPO=/path/to/PRG32
   export PRG32_ARCHITECTURE=qemu
   scripts/build.sh
   ```

**Code to add:**

Add the first version of `src/debris.c`.

```c
#include "prg32.h"

/* A cartridge is called repeatedly by the PRG32 runtime.  These variables are
   deliberately global because the runtime calls init, update, draw, and
   shutdown as separate functions. */
static unsigned long frame;

void debris_init(void) {
    /* Configure the surrounding PRG32 interface before the first frame. */
    prg32_gfx_set_fullscreen(0);
    prg32_band_set_mode(PRG32_BAND_TOP, PRG32_BAND_MODE_FPS);
    prg32_band_set_mode(PRG32_BAND_BOTTOM, PRG32_BAND_MODE_CUSTOM);
    prg32_band_set_game_info("DEBRIS");
    frame = 0;
}

void debris_update(void) {
    /* The first program has no game rules yet; it only counts frames so that
       students can see that update is the simulation step. */
    frame++;
}

void debris_draw(void) {
    /* Drawing is kept separate from update.  This habit prevents visual code
       from accidentally changing the game model. */
    prg32_gfx_clear(PRG32_COLOR_BLACK);
    prg32_gfx_text8(72, 42, "DEBRIS", PRG32_COLOR_YELLOW, PRG32_COLOR_BLACK);
    prg32_gfx_text8(48, 66, "A FALLING BLOCK GAME",
                    PRG32_COLOR_CYAN, PRG32_COLOR_BLACK);
    prg32_band_set_text(PRG32_BAND_BOTTOM, "LAB 2: TITLE SCREEN");
}

void debris_shutdown(void) {
    /* Always leave the platform in a polite state.  Later laboratories will
       restore more settings here. */
    prg32_band_set_game_info("");
    prg32_band_set_text(PRG32_BAND_BOTTOM, "");
}
```

Use or create `scripts/build.sh` so that the build tool knows the entry prefix
and source file.

```sh
#!/usr/bin/env bash
set -euo pipefail

# The project is easiest to build when PRG32 is beside this repository, but the
# environment variable lets a laboratory machine use a different location.
repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="${PRG32_REPO:-"$repo_dir/../PRG32"}"
architecture="${PRG32_ARCHITECTURE:-qemu}"

mkdir -p "$repo_dir/dist"

python3 "$prg32_repo/tools/prg32_game.py" build \
  "$repo_dir/src/debris.c" \
  --entry-prefix debris \
  --name debris \
  --portable \
  --out "$repo_dir/dist/debris-$architecture.prg32"
```

**Expected product:** A cartridge that builds and shows a title screen.

**Debug focus:** If the cartridge does not build, check the entry prefix passed
to the build tool. The source functions and build script must agree on the
`debris` prefix.

## Laboratory 3: State Machine and Input Edges

**Estimated duration:** 1 hour

**Objective:** Add title, playing, paused, and game-over states.

**Concepts:** Finite state machines, input polling, pressed-edge detection.

**Implementation steps:**

1. Define a `game_state_t` enum with title, playing, paused, and game-over
   states.
2. Store the current state and previous input in a global game struct.
3. In `debris_update`, read input with `prg32_input_read_menu`.
4. Compute newly pressed buttons with:

   ```c
   uint32_t pressed = input & ~g.last_input;
   ```

5. Use `SELECT` to start, pause, resume, or restart according to the current
   state.
6. Draw a different screen or overlay for each state.

**Code to add:**

Extend `src/debris.c` with explicit states and edge-triggered input.

```c
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAME_OVER
} game_state_t;

typedef struct {
    uint8_t state;
    uint32_t last_input;
    uint32_t frame;
} debris_t;

static debris_t g;

static void reset_game(void) {
    /* Reset only the game model.  Do not configure hardware here; that belongs
       in init. */
    g.state = STATE_PLAYING;
}

void debris_init(void) {
    prg32_gfx_set_fullscreen(0);
    prg32_band_set_mode(PRG32_BAND_TOP, PRG32_BAND_MODE_FPS);
    prg32_band_set_mode(PRG32_BAND_BOTTOM, PRG32_BAND_MODE_CUSTOM);
    prg32_band_set_game_info("DEBRIS");
    g.state = STATE_TITLE;
    g.last_input = 0;
    g.frame = 0;

    /* Prevent a button used to launch the cartridge from also starting the
       game on the first frame. */
    prg32_input_wait_released(PRG32_BTN_A | PRG32_BTN_B | PRG32_BTN_SELECT);
}

void debris_update(void) {
    uint32_t input = prg32_input_read_menu();

    /* pressed is true for one frame only.  This is the difference between
       "the button is down" and "the button has just been pressed". */
    uint32_t pressed = input & ~g.last_input;
    g.frame++;

    if (g.state == STATE_TITLE && (pressed & PRG32_BTN_SELECT)) {
        reset_game();
    } else if (g.state == STATE_PLAYING && (pressed & PRG32_BTN_SELECT)) {
        g.state = STATE_PAUSED;
    } else if (g.state == STATE_PAUSED && (pressed & PRG32_BTN_SELECT)) {
        g.state = STATE_PLAYING;
    } else if (g.state == STATE_GAME_OVER && (pressed & PRG32_BTN_SELECT)) {
        reset_game();
    }

    g.last_input = input;
}
```

Then update `debris_draw` in the same file.

```c
void debris_draw(void) {
    prg32_gfx_clear(PRG32_COLOR_BLACK);

    if (g.state == STATE_TITLE) {
        prg32_gfx_text8(72, 42, "DEBRIS", PRG32_COLOR_YELLOW, PRG32_COLOR_BLACK);
        prg32_gfx_text8(56, 154, "SELECT STARTS",
                        PRG32_COLOR_WHITE, PRG32_COLOR_BLACK);
    } else if (g.state == STATE_PLAYING) {
        prg32_gfx_text8(96, 92, "PLAYING",
                        PRG32_COLOR_GREEN, PRG32_COLOR_BLACK);
    } else if (g.state == STATE_PAUSED) {
        prg32_gfx_text8(104, 92, "PAUSED",
                        PRG32_COLOR_YELLOW, PRG32_COLOR_BLACK);
    } else {
        prg32_gfx_text8(96, 92, "GAME OVER",
                        PRG32_COLOR_RED, PRG32_COLOR_BLACK);
    }

    prg32_band_set_text(PRG32_BAND_BOTTOM, "SELECT START/PAUSE/RESUME");
}
```

**Expected product:** A cartridge whose screens change only when the player
presses the correct button.

**Debug focus:** Holding a button should not repeatedly toggle pause. If it does,
the program is using level input where it needs edge input.

## Laboratory 4: Board Model and Coordinates

**Estimated duration:** 1 hour

**Objective:** Represent the game board independently from the screen.

**Concepts:** Arrays, logical coordinates, pixel coordinates, constants.

**Implementation steps:**

1. Define board constants:

   ```c
   #define BOARD_W 10
   #define BOARD_H 20
   #define CELL 8
   ```

2. Store the board as:

   ```c
   uint8_t board[BOARD_H][BOARD_W];
   ```

3. Write a reset function that clears every cell.
4. Draw the board frame and each occupied cell.
5. Convert logical cells to pixels with:

   ```c
   screen_x = BOARD_X + x * CELL;
   screen_y = BOARD_Y + y * CELL;
   ```

**Code to add:**

Add board constants and board storage to `src/debris.c`.

```c
#define BOARD_W 10
#define BOARD_H 20
#define CELL 8
#define BOARD_X 84
#define BOARD_Y 20

typedef struct {
    uint8_t state;
    uint32_t last_input;
    uint32_t frame;

    /* board[y][x] stores the logical contents of the board.  Zero means empty;
       later, non-zero values will identify piece colors. */
    uint8_t board[BOARD_H][BOARD_W];
} debris_t;
```

Add a board reset helper in the same file.

```c
static void clear_board(void) {
    for (int y = 0; y < BOARD_H; ++y) {
        for (int x = 0; x < BOARD_W; ++x) {
            g.board[y][x] = 0;
        }
    }
}

static void reset_game(void) {
    clear_board();
    g.state = STATE_PLAYING;
}
```

Add drawing helpers to `src/debris.c`.

```c
static void draw_block(int x, int y, uint16_t color) {
    /* x and y are pixel coordinates here, not board coordinates. */
    prg32_gfx_rect(x, y, CELL - 1, CELL - 1, color);
    prg32_gfx_rect(x + 1, y + 1, CELL - 3, 1, PRG32_COLOR_WHITE);
    prg32_gfx_pixel(x + CELL - 2, y + CELL - 2, PRG32_COLOR_BLACK);
}

static void draw_board(void) {
    prg32_gfx_rect(BOARD_X - 4, BOARD_Y - 4,
                   BOARD_W * CELL + 8, BOARD_H * CELL + 8,
                   PRG32_COLOR_BLUE);
    prg32_gfx_rect(BOARD_X, BOARD_Y,
                   BOARD_W * CELL, BOARD_H * CELL,
                   PRG32_COLOR_BLACK);

    for (int y = 0; y < BOARD_H; ++y) {
        for (int x = 0; x < BOARD_W; ++x) {
            if (g.board[y][x]) {
                draw_block(BOARD_X + x * CELL,
                           BOARD_Y + y * CELL,
                           PRG32_COLOR_CYAN);
            }
        }
    }
}
```

Call `draw_board()` from the playing branch of `debris_draw`.

**Expected product:** A visible empty board, correctly aligned and stable on
screen.

**Debug focus:** Most board bugs are coordinate bugs. Inspect whether a value is
a cell coordinate or a pixel coordinate before changing arithmetic.

## Laboratory 5: Pieces, Rotations, and Collision Tests

**Estimated duration:** 1 hour

**Objective:** Add the seven falling pieces and determine whether a placement is
legal.

**Concepts:** Constant tables, rotation states, collision detection, boundary
conditions.

**Implementation steps:**

1. Encode the seven pieces as four blocks across four rotations.
2. Store the active piece, rotation, `x`, and `y` in the game state.
3. Implement a `fits(piece, rot, x, y)` function that checks:
   - Left and right walls.
   - Bottom of the board.
   - Occupied board cells.
   - Negative `y` positions during spawn.
4. Draw the active piece on top of the board.
5. Spawn a piece at the top center.

**Code to add:**

Add shape data and colors to `src/debris.c`.

```c
static const uint16_t colors[8] = {
    PRG32_COLOR_BLACK,
    PRG32_COLOR_CYAN,
    PRG32_COLOR_BLUE,
    PRG32_COLOR_YELLOW,
    PRG32_COLOR_GREEN,
    PRG32_COLOR_MAGENTA,
    PRG32_COLOR_RED,
    PRG32_COLOR_WHITE,
};

/* shape[piece][rotation][block][coordinate].
   Each piece has four blocks.  Each block has x and y offsets inside a 4 by 4
   local grid.  This table is data, not logic: it makes rotations explicit. */
static const int8_t shape[7][4][4][2] = {
    {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}},
    {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}},
    {{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}},
    {{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}},
    {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}},
    {{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}},
    {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}},
};
```

Extend the game state in `src/debris.c`.

```c
typedef struct {
    uint8_t state;
    uint32_t last_input;
    uint32_t frame;
    uint8_t board[BOARD_H][BOARD_W];

    /* The active piece is stored in board coordinates. */
    int piece;
    int rot;
    int x;
    int y;
} debris_t;
```

Add collision, spawn, and piece drawing helpers.

```c
static int fits(int piece, int rot, int x, int y) {
    for (int i = 0; i < 4; ++i) {
        int px = x + shape[piece][rot][i][0];
        int py = y + shape[piece][rot][i][1];

        if (px < 0 || px >= BOARD_W || py >= BOARD_H) return 0;

        /* py may be negative while a new piece enters from above the board.
           Negative rows are legal for collision but invisible when drawing. */
        if (py >= 0 && g.board[py][px]) return 0;
    }
    return 1;
}

static void spawn(void) {
    g.piece = 0;   /* Start with the I piece while learning; randomness comes later. */
    g.rot = 0;
    g.x = 3;
    g.y = -2;
    if (!fits(g.piece, g.rot, g.x, g.y)) {
        g.state = STATE_GAME_OVER;
    }
}

static void draw_piece(int piece, int rot, int x, int y) {
    for (int i = 0; i < 4; ++i) {
        int px = x + shape[piece][rot][i][0];
        int py = y + shape[piece][rot][i][1];
        if (py >= 0) {
            draw_block(BOARD_X + px * CELL,
                       BOARD_Y + py * CELL,
                       colors[piece + 1]);
        }
    }
}
```

Call `spawn()` from `reset_game()` and `draw_piece()` after `draw_board()`.

**Expected product:** A static falling piece is visible and never drawn outside
legal board positions.

**Debug focus:** Negative spawn rows are allowed for collision tests but should
not be drawn as visible blocks.

## Laboratory 6: Movement, Rotation, and Wall Kicks

**Estimated duration:** 1 hour

**Objective:** Make the current piece controllable.

**Concepts:** Command handling, tentative movement, reversible checks, small
rule refinements.

**Implementation steps:**

1. Add left and right movement with `fits` checks.
2. Add clockwise rotation on `A`.
3. Add a small wall-kick table, for example `0, -1, 1, -2, 2`, so pieces can
   rotate near walls.
4. Add a hard drop on `UP`.
5. Add a soft drop when `DOWN` is held.

**Code to add:**

Add movement functions to `src/debris.c`.

```c
static int move_piece(int dx, int dy) {
    /* Movement is tentative: test the desired position before changing state. */
    if (fits(g.piece, g.rot, g.x + dx, g.y + dy)) {
        g.x += dx;
        g.y += dy;
        return 1;
    }
    return 0;
}

static void rotate_piece(int dir) {
    int next = (g.rot + dir + 4) & 3;
    static const int8_t kicks[] = {0, -1, 1, -2, 2};

    /* Wall kicks try nearby x offsets so that rotation near a wall feels fair. */
    for (int i = 0; i < 5; ++i) {
        if (fits(g.piece, next, g.x + kicks[i], g.y)) {
            g.rot = next;
            g.x += kicks[i];
            return;
        }
    }
}

static void hard_drop(void) {
    while (fits(g.piece, g.rot, g.x, g.y + 1)) {
        g.y++;
    }
    /* Locking will be implemented in the next lab.  For now, the piece simply
       stops at the lowest legal row. */
}
```

Use those functions in the playing branch of `debris_update`.

```c
if (g.state == STATE_PLAYING) {
    if (pressed & PRG32_BTN_LEFT) move_piece(-1, 0);
    if (pressed & PRG32_BTN_RIGHT) move_piece(1, 0);
    if (pressed & PRG32_BTN_A) rotate_piece(1);
    if (pressed & PRG32_BTN_UP) hard_drop();

    /* Soft drop is level input, not edge input: holding DOWN should keep moving
       the piece down across multiple frames. */
    if (input & PRG32_BTN_DOWN) move_piece(0, 1);
}
```

**Expected product:** The player can move, rotate, hard-drop, and soft-drop a
piece without corrupting the board.

**Debug focus:** A movement function should first ask "would the move fit?" and
only then modify `g.x`, `g.y`, or `g.rot`.

## Laboratory 7: Gravity, Locking, and Line Clearing

**Estimated duration:** 1 hour

**Objective:** Complete the central game loop.

**Concepts:** Timers, frame-based simulation, locking, row compaction.

**Implementation steps:**

1. Add a frame counter or drop timer.
2. Every fixed number of frames, attempt to move the active piece down.
3. If downward movement fails repeatedly, lock the piece into the board.
4. Scan rows from bottom to top.
5. When a row is full, shift all rows above it down and clear the top row.
6. Spawn the next piece after locking.
7. Enter game-over state if a newly spawned piece does not fit.

**Code to add:**

Extend the state in `src/debris.c`.

```c
typedef struct {
    uint8_t state;
    uint32_t last_input;
    uint32_t frame;
    uint8_t board[BOARD_H][BOARD_W];
    int piece;
    int rot;
    int x;
    int y;

    uint8_t drop_timer;
    uint8_t lock_delay;
} debris_t;
```

Add locking and line clearing.

```c
static void clear_lines(void) {
    for (int y = BOARD_H - 1; y >= 0; --y) {
        int full = 1;
        for (int x = 0; x < BOARD_W; ++x) {
            if (!g.board[y][x]) {
                full = 0;
                break;
            }
        }

        if (!full) continue;

        /* Move every row above the cleared row down by one. */
        for (int yy = y; yy > 0; --yy) {
            for (int x = 0; x < BOARD_W; ++x) {
                g.board[yy][x] = g.board[yy - 1][x];
            }
        }

        for (int x = 0; x < BOARD_W; ++x) g.board[0][x] = 0;

        /* Recheck this same y after shifting, otherwise two adjacent full rows
           can leave one uncleared. */
        ++y;
    }
}

static void lock_piece(void) {
    for (int i = 0; i < 4; ++i) {
        int px = g.x + shape[g.piece][g.rot][i][0];
        int py = g.y + shape[g.piece][g.rot][i][1];
        if (py >= 0 && py < BOARD_H && px >= 0 && px < BOARD_W) {
            g.board[py][px] = (uint8_t)(g.piece + 1);
        }
    }

    clear_lines();
    spawn();
}
```

Update `spawn()` and the playing update code.

```c
static void spawn(void) {
    g.piece = (int)((g.frame / 17u) % 7u); /* Temporary variety before RNG. */
    g.rot = 0;
    g.x = 3;
    g.y = -2;
    g.drop_timer = 0;
    g.lock_delay = 0;

    if (!fits(g.piece, g.rot, g.x, g.y)) {
        g.state = STATE_GAME_OVER;
    }
}

static void update_play(uint32_t input, uint32_t pressed) {
    if (pressed & PRG32_BTN_LEFT) move_piece(-1, 0);
    if (pressed & PRG32_BTN_RIGHT) move_piece(1, 0);
    if (pressed & PRG32_BTN_A) rotate_piece(1);
    if (pressed & PRG32_BTN_UP) {
        hard_drop();
        lock_piece();
        return;
    }

    uint8_t fall_limit = (input & PRG32_BTN_DOWN) ? 3 : 34;
    if (++g.drop_timer >= fall_limit) {
        g.drop_timer = 0;
        if (!move_piece(0, 1)) {
            if (++g.lock_delay >= 3) lock_piece();
        } else {
            g.lock_delay = 0;
        }
    }
}
```

**Expected product:** A playable falling-block loop with line clears and game
over.

**Debug focus:** When clearing rows from bottom to top, recheck the same row
after shifting. Otherwise consecutive full rows may be skipped.

## Laboratory 8: Score, Levels, Queue, and Hold

**Estimated duration:** 1 hour

**Objective:** Add the systems that make the game feel complete.

**Concepts:** Game economy, user feedback, random generation, state invariants.

**Implementation steps:**

1. Add score values for one, two, three, and four line clears.
2. Increase level after a fixed number of cleared lines.
3. Make gravity faster as the level increases, with a minimum fall interval.
4. Add a three-piece next queue.
5. Add hold/swap on `B`.
6. Prevent repeated hold use on the same falling piece.

**Code to add:**

Extend `src/debris.c` with scoring, queue, hold, and random state.

```c
typedef struct {
    uint8_t state;
    uint32_t last_input;
    uint32_t frame;
    uint8_t board[BOARD_H][BOARD_W];
    int piece;
    int rot;
    int x;
    int y;
    uint8_t drop_timer;
    uint8_t lock_delay;

    uint32_t rng;
    uint32_t score;
    uint16_t lines;
    uint8_t level;
    int hold;
    int hold_used;
    int queue[3];
} debris_t;
```

Add helper functions.

```c
static uint32_t rnd(void) {
    /* A linear congruential generator is small and deterministic enough for a
       classroom game.  It is not cryptography; it is controlled variety. */
    g.rng = g.rng * 1664525u + 1013904223u;
    return g.rng;
}

static int next_piece(void) {
    return (int)(rnd() % 7u);
}

static void add_score(int cleared, int hard_cells) {
    static const uint16_t line_score[5] = {0, 100, 300, 500, 800};
    if (cleared > 0) {
        g.score += (uint32_t)line_score[cleared] * (uint32_t)g.level;
        g.lines = (uint16_t)(g.lines + cleared);
        g.level = (uint8_t)(1 + g.lines / 8);
        if (g.level > 12) g.level = 12;
    }
    g.score += (uint32_t)hard_cells * 2u;
}

static int fall_frames(void) {
    int speed = 34 - (int)(g.level - 1) * 2;
    return speed < 6 ? 6 : speed;
}
```

Replace the temporary `spawn()` with a queue-based version.

```c
static void spawn(void) {
    g.piece = g.queue[0];
    g.queue[0] = g.queue[1];
    g.queue[1] = g.queue[2];
    g.queue[2] = next_piece();
    g.rot = 0;
    g.x = 3;
    g.y = -2;
    g.hold_used = 0;
    g.drop_timer = 0;
    g.lock_delay = 0;

    if (!fits(g.piece, g.rot, g.x, g.y)) {
        g.state = STATE_GAME_OVER;
    }
}
```

Initialize the new systems in `reset_game()`.

```c
static void reset_game(void) {
    clear_board();
    g.rng ^= prg32_ticks_ms() + 0x6d2b79f5u;
    g.score = 0;
    g.lines = 0;
    g.level = 1;
    g.hold = -1;
    g.hold_used = 0;
    g.queue[0] = next_piece();
    g.queue[1] = next_piece();
    g.queue[2] = next_piece();
    g.state = STATE_PLAYING;
    spawn();
}
```

Add hold and improved hard drop.

```c
static void hold_piece(void) {
    if (g.hold_used) return;

    int old = g.piece;
    if (g.hold < 0) {
        g.hold = old;
        spawn();
    } else {
        g.piece = g.hold;
        g.hold = old;
        g.rot = 0;
        g.x = 3;
        g.y = -2;
        if (!fits(g.piece, g.rot, g.x, g.y)) g.state = STATE_GAME_OVER;
    }

    g.hold_used = 1;
}

static void hard_drop(void) {
    int cells = 0;
    while (fits(g.piece, g.rot, g.x, g.y + 1)) {
        g.y++;
        cells++;
    }
    add_score(0, cells);
    lock_piece();
}
```

Finally, call `hold_piece()` on `B`, use `fall_frames()`, and increment score
for soft drop.

```c
if (pressed & PRG32_BTN_B) hold_piece();
uint8_t limit = (input & PRG32_BTN_DOWN) ? 3 : (uint8_t)fall_frames();
```

**Expected product:** The game shows score, level, lines, hold, and next pieces.

**Debug focus:** The hold feature has an invariant: each falling piece can use
hold at most once. Reset that flag only when a new piece spawns.

## Laboratory 9: Rendering and Interface Polish

**Estimated duration:** 1 hour

**Objective:** Improve visual clarity without changing game rules.

**Concepts:** Visual hierarchy, readable feedback, screen composition, embedded
constraints.

**Implementation steps:**

1. Draw a stable playfield frame.
2. Use distinct colors for each piece type.
3. Add panels for score, level, lines, hold, and next pieces.
4. Add title, pause, and game-over overlays.
5. Add a short instruction string in the bottom band.
6. Test that all text fits on the target screen.

**Code to add:**

Add text helpers to `src/debris.c` so students avoid `sprintf` and learn bounded
string construction.

```c
static void append_char(char *dst, int cap, int *pos, char ch) {
    if (*pos < cap - 1) {
        dst[*pos] = ch;
        *pos += 1;
        dst[*pos] = '\0';
    }
}

static void append_text(char *dst, int cap, int *pos, const char *src) {
    while (src && *src) append_char(dst, cap, pos, *src++);
}

static void append_uint(char *dst, int cap, int *pos, uint32_t value) {
    char tmp[12];
    int count = 0;
    do {
        tmp[count++] = (char)('0' + (value % 10u));
        value /= 10u;
    } while (value && count < (int)sizeof(tmp));
    while (count > 0) append_char(dst, cap, pos, tmp[--count]);
}

static void make_line(char *dst, int cap, const char *label, uint32_t value) {
    int pos = 0;
    dst[0] = '\0';
    append_text(dst, cap, &pos, label);
    append_uint(dst, cap, &pos, value);
}
```

First adapt `draw_block` and `draw_piece` so the same drawing code can be used
for the main board and for smaller preview pieces.

```c
static void draw_block(int x, int y, uint16_t color, int small) {
    int size = small ? 6 : CELL;
    prg32_gfx_rect(x, y, size - 1, size - 1, color);
    prg32_gfx_rect(x + 1, y + 1, size - 3, 1, PRG32_COLOR_WHITE);
    prg32_gfx_pixel(x + size - 2, y + size - 2, PRG32_COLOR_BLACK);
}

static void draw_piece(int piece, int rot, int x, int y, int cell) {
    if (piece < 0) return;
    for (int i = 0; i < 4; ++i) {
        int px = x + shape[piece][rot][i][0] * cell;
        int py = y + shape[piece][rot][i][1] * cell;
        draw_block(px, py, colors[piece + 1], cell < CELL);
    }
}
```

Then add interface drawing functions.

```c
static void draw_panel_text(int x, int y, const char *text, uint16_t fg) {
    prg32_gfx_text8(x, y, text, fg, PRG32_COLOR_BLACK);
}

static void draw_stats(void) {
    char line[32];
    draw_panel_text(8, 8, "DEBRIS", PRG32_COLOR_YELLOW);
    make_line(line, sizeof(line), "SCORE ", g.score);
    draw_panel_text(188, 16, line, PRG32_COLOR_WHITE);
    make_line(line, sizeof(line), "LEVEL ", g.level);
    draw_panel_text(188, 32, line, PRG32_COLOR_CYAN);
    make_line(line, sizeof(line), "LINES ", g.lines);
    draw_panel_text(188, 48, line, PRG32_COLOR_GREEN);

    draw_panel_text(28, 42, "HOLD", PRG32_COLOR_CYAN);
    draw_piece(g.hold, 0, 28, 58, 6);
    draw_panel_text(204, 76, "NEXT", PRG32_COLOR_CYAN);
    for (int n = 0; n < 3; ++n) {
        draw_piece(g.queue[n], 0, 204, 92 + n * 30, 6);
    }
}

static void draw_game(void) {
    prg32_gfx_clear(PRG32_COLOR_BLACK);
    draw_board();
    draw_piece(g.piece, g.rot, BOARD_X + g.x * CELL, BOARD_Y + g.y * CELL, CELL);
    draw_stats();

    if (g.state == STATE_PAUSED) {
        prg32_gfx_rect(72, 84, 176, 34, 0x2104);
        draw_panel_text(112, 94, "PAUSED", PRG32_COLOR_YELLOW);
        draw_panel_text(88, 108, "SELECT TO RESUME", PRG32_COLOR_WHITE);
    }
}
```

After changing `draw_block`, update earlier board-drawing calls to pass `0` for
normal board cells. This makes the compiler a useful checklist for every call
site affected by the new parameter.

**Expected product:** A game that a new player can understand without external
instructions.

**Debug focus:** Rendering should read from game state. It should not change
score, position, board contents, queue, or state transitions.

## Laboratory 10: Sound and Platform Etiquette

**Estimated duration:** 1 hour

**Objective:** Add simple sound effects and restore platform state on exit.

**Concepts:** Optional subsystems, generated assets, resource cleanup.

**Implementation steps:**

1. Initialize audio in `debris_init`.
2. Guard all audio calls behind an `audio_ready` flag.
3. Add short beeps for movement, rotation, pause, and restart.
4. Add generated sample arrays for lock and line-clear effects.
5. Save previous display band and fullscreen settings during initialization.
6. Restore those settings in `debris_shutdown`.

**Code to add:**

Add saved platform settings near the other global variables in `src/debris.c`.

```c
static int saved_fullscreen;
static prg32_band_mode_t saved_top;
static prg32_band_mode_t saved_bottom;
```

Add one field to the existing `debris_t` struct.

```c
typedef struct {
    /* Keep all fields from the previous labs.  Add this one new field. */
    uint8_t audio_ready;
} debris_t;
```

Add generated sound data and guarded sound helpers.

```c
static const uint8_t thump_sample[] = {
    128, 210, 250, 238, 190, 134, 86, 54,
    42, 50, 72, 100, 126, 145, 154, 154,
};

static const uint8_t crackle_sample[] = {
    128, 240, 24, 210, 70, 186, 52, 225,
    36, 172, 91, 235, 64, 154, 104, 202,
};

static void sound_beep(int freq, int ms) {
    if (!g.audio_ready) return;
    prg32_audio_beep(freq, ms);
}

static void sound_sample(const uint8_t *sample, int len, int rate) {
    if (!g.audio_ready) return;
    prg32_audio_sample_u8(sample, (uint32_t)len, rate);
}
```

Update lifecycle and rule functions.

```c
void debris_init(void) {
    saved_fullscreen = prg32_gfx_fullscreen_enabled();
    saved_top = prg32_band_mode(PRG32_BAND_TOP);
    saved_bottom = prg32_band_mode(PRG32_BAND_BOTTOM);

    prg32_gfx_set_fullscreen(0);
    prg32_band_set_mode(PRG32_BAND_TOP, PRG32_BAND_MODE_FPS);
    prg32_band_set_mode(PRG32_BAND_BOTTOM, PRG32_BAND_MODE_CUSTOM);
    prg32_band_set_game_info("DEBRIS");

    g.rng = 0xdeb815u ^ prg32_ticks_ms();
    g.state = STATE_TITLE;
    g.audio_ready = prg32_audio_init(NULL) ? 1 : 0;
    if (g.audio_ready) prg32_audio_set_master_volume(160);
}

static void lock_piece(void) {
    /* Existing board-copy code appears here. */
    sound_sample(thump_sample, sizeof(thump_sample), 9000);
    clear_lines();
    spawn();
}

void debris_shutdown(void) {
    prg32_band_set_game_info("");
    prg32_band_set_mode(PRG32_BAND_TOP, saved_top);
    prg32_band_set_mode(PRG32_BAND_BOTTOM, saved_bottom);
    if (g.audio_ready) prg32_audio_stop_all();
    prg32_gfx_set_fullscreen(saved_fullscreen);
}
```

Place `sound_beep` calls after successful movement, rotation, pause, resume, and
restart. Place `sound_sample(crackle_sample, sizeof(crackle_sample), 15000)`
after a line clear.

**Expected product:** The game has feedback sounds and leaves the PRG32 shell in
a clean state when it exits.

**Debug focus:** Audio must be treated as optional. A failure to initialize
audio should not prevent the game from running.

## Laboratory 11: Metadata, Assets, and Legal Readiness

**Estimated duration:** 1 hour

**Objective:** Prepare the cartridge for distribution.

**Concepts:** Metadata schemas, licensing, authorship, asset provenance.

**Implementation steps:**

1. Complete `metadata/metadata.json` with:
   - Stable cartridge identifier.
   - Title, version, summary, and description.
   - Authors, license, homepage, and repository.
   - Runtime architectures.
   - Asset block references.
2. Complete `metadata/colophon.json` with:
   - Developer and author roles.
   - Release date and license.
   - Controls and start prompt.
   - Acknowledgement that graphics and sounds are original or generated.
3. Complete `metadata/manifest.json` for store bundling.
4. Provide `assets/icon.png` and `assets/screenshot.png`.
5. Confirm that no third-party copyrighted art or audio is included unless it is
   licensed and credited correctly.

**Code to add:**

Create or update `metadata/metadata.json`.

```json
{
  "abi": "prg32-metadata-1.0",
  "id": "org.uniparthenope.prg32.debris",
  "title": "Debris",
  "version": "1.0.0",
  "summary": "A fully playable multi-level falling-block puzzle game for PRG32.",
  "description": "Debris is a Tetris-inspired PRG32 cartridge with movement, rotation, hard and soft drops, hold, next-piece queue, scoring, line clears, pause, restart, procedural graphics, and generated sound effects.",
  "authors": [
    {
      "name": "Student Team",
      "affiliation": "Course Institution"
    }
  ],
  "license": "MIT",
  "homepage": "https://github.com/riscv-prg32/Debris",
  "repository": "https://github.com/riscv-prg32/Debris",
  "tags": ["game", "puzzle", "arcade", "riscv", "prg32"],
  "runtime": {
    "platform": "PRG32",
    "isa": "RV32I",
    "architectures": [
      {
        "id": "esp32c6",
        "label": "ESP32-C6 hardware",
        "target": "esp32c6",
        "display": "ili9341",
        "isa": "RV32I"
      },
      {
        "id": "qemu",
        "label": "QEMU virtual screen",
        "target": "esp32c3",
        "display": "qemu-rgb",
        "isa": "RV32I"
      }
    ]
  },
  "assets": {
    "icon": {"block": "ICON", "mime": "image/png"},
    "screenshot": {"block": "SCRN", "mime": "image/png", "optional": true}
  },
  "colophon": {
    "block": "COLO",
    "abi": "prg32-colophon-1.0"
  }
}
```

JSON does not allow comments. Teach students to explain metadata choices in
`docs/design-notes.md`, not inside the JSON files.

Create or update `metadata/manifest.json`.

```json
{
  "abi": "prg32-metadata-1.0",
  "id": "org.uniparthenope.prg32.debris",
  "title": "Debris",
  "version": "1.0.0",
  "summary": "A fully playable multi-level falling-block puzzle game for PRG32.",
  "tags": ["game", "puzzle", "arcade", "riscv", "prg32"],
  "assets": {
    "icon": "icon.png",
    "splash": "screenshot.png"
  },
  "architectures": [
    {"id": "esp32c6", "file": "debris-esp32c6.prg32"},
    {"id": "qemu", "file": "debris-qemu.prg32"}
  ]
}
```

Create or update `metadata/colophon.json`.

```json
{
  "abi": "prg32-colophon-1.0",
  "title": "Debris",
  "subtitle": "A multi-level falling-block puzzle game",
  "version": "1.0.0",
  "release_date": "2026-06-11",
  "developer": {
    "name": "Course Team",
    "url": "https://github.com/riscv-prg32/Debris"
  },
  "license": "MIT",
  "acknowledgements": [
    "Original procedural graphics and generated sound effects. No third-party copyrighted art or audio assets are used."
  ],
  "controls": [
    {"input": "LEFT / RIGHT", "action": "Move the falling piece"},
    {"input": "A", "action": "Rotate clockwise"},
    {"input": "UP", "action": "Hard drop"},
    {"input": "DOWN", "action": "Soft drop"},
    {"input": "B", "action": "Hold or swap the current piece"},
    {"input": "SELECT", "action": "Start, pause, resume, or restart"}
  ],
  "start_prompt": "Press SELECT to start"
}
```

Add or verify binary assets:

```text
assets/icon.png        # Square icon shown by launchers and the store.
assets/screenshot.png  # Gameplay screenshot used by the store listing.
```

**Expected product:** A cartridge with complete publication metadata and a clear
asset provenance story.

**Debug focus:** Metadata errors are release blockers. Validate spelling,
version numbers, filenames, and architecture identifiers before packaging.

## Laboratory 12: Build, Publish, and Present

**Estimated duration:** 1 hour

**Objective:** Produce a Cartridge Store bundle and present the work as a
finished software artifact.

**Concepts:** Release build, multi-target packaging, publication checklist,
technical communication.

**Implementation steps:**

1. Build the ESP32-C6 cartridge:

   ```sh
   export PRG32_REPO=/path/to/PRG32
   export PRG32_ARCHITECTURE=esp32c6
   scripts/build.sh
   ```

2. Build the QEMU cartridge:

   ```sh
   export PRG32_ARCHITECTURE=qemu
   scripts/build.sh
   ```

3. Package the store bundle:

   ```sh
   scripts/pack-store-bundle.sh
   ```

4. Confirm that the output exists:

   ```text
   dist/debris-store-bundle.zip
   ```

5. Upload the bundle to the Cartridge Store using the store publication
   procedure adopted by the course or project maintainers.
6. Verify the store page:
   - Title and summary are correct.
   - Icon and screenshot are visible.
   - Both architectures are listed.
   - Download and launch work on the expected target.
7. Give a five-minute demonstration:
   - Explain one design decision.
   - Show the code-deploy-debug cycle.
   - Demonstrate one corrected bug or one tested edge case.

**Code to add:**

Use this release-quality `scripts/build.sh`.

```sh
#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="${PRG32_REPO:-"$repo_dir/../PRG32"}"
firmware_elf="${1:-"${PRG32_FIRMWARE_ELF:-"$prg32_repo/build/PRG32.elf"}"}"
architecture="${PRG32_ARCHITECTURE:-esp32c6}"
portable="${PRG32_PORTABLE:-1}"
game_tool="$prg32_repo/tools/prg32_game.py"

build_args=()
if [[ "$portable" == "1" || "$portable" == "true" || "$portable" == "yes" ]]; then
  build_args+=(--portable)
else
  # Legacy builds are tied to a specific firmware image.  Use them only when the
  # course infrastructure explicitly requires that format.
  build_args+=(--firmware-elf "$firmware_elf" --legacy-absolute-imports)
fi

mkdir -p "$repo_dir/dist"

python3 "$game_tool" build \
  "$repo_dir/src/debris.c" \
  --entry-prefix debris \
  --name debris \
  --out "$repo_dir/dist/debris-$architecture.raw.prg32" \
  "${build_args[@]}"

python3 "$game_tool" attach-metadata \
  "$repo_dir/dist/debris-$architecture.raw.prg32" \
  --out "$repo_dir/dist/debris-$architecture.prg32" \
  --metadata "$repo_dir/metadata/metadata.json" \
  --icon "$repo_dir/assets/icon.png" \
  --screenshot "$repo_dir/assets/screenshot.png" \
  --colophon "$repo_dir/metadata/colophon.json" \
  --architecture "$architecture"

echo "$repo_dir/dist/debris-$architecture.prg32"
```

Use this `scripts/pack-store-bundle.sh`.

```sh
#!/usr/bin/env bash
set -euo pipefail

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
prg32_repo="${PRG32_REPO:-"$repo_dir/../PRG32"}"
stage_dir="$repo_dir/dist/store-bundle"

# The staging directory is rebuilt from scratch so that old cartridge files
# cannot accidentally enter the store bundle.
rm -rf "$stage_dir"
mkdir -p "$stage_dir"

cp "$repo_dir/metadata/manifest.json" "$stage_dir/manifest.json"
cp "$repo_dir/assets/icon.png" "$stage_dir/icon.png"
cp "$repo_dir/assets/screenshot.png" "$stage_dir/screenshot.png"
cp "$repo_dir/dist/debris-esp32c6.prg32" "$stage_dir/debris-esp32c6.prg32"
cp "$repo_dir/dist/debris-qemu.prg32" "$stage_dir/debris-qemu.prg32"

python3 "$prg32_repo/tools/prg32_game.py" pack-bundle \
  --manifest "$stage_dir/manifest.json" \
  --out "$repo_dir/dist/debris-store-bundle.zip"

echo "$repo_dir/dist/debris-store-bundle.zip"
```

Update `README.md` so that future students and reviewers can find the teaching
material.

```md
## Documentation

- [Build and publish guide](docs/build-and-publish.md)
- [Step-by-step teaching tutorial](docs/teaching-tutorial.md)
```

**Expected product:** A published or publication-ready Cartridge Store bundle.

**Debug focus:** A bundle can fail even when the game builds. Check filenames in
`metadata/manifest.json` against files copied by `scripts/pack-store-bundle.sh`.

## Assessment Rubric

Use this rubric for formative feedback and final grading.

| Criterion | Excellent | Satisfactory | Needs Work |
| --- | --- | --- | --- |
| Game model | Board, pieces, queue, hold, score, and levels are coherent and easy to explain. | Core loop works with minor rule or clarity issues. | State is fragile, incomplete, or mixed with rendering. |
| Code quality | Functions are small, named well, and organized by responsibility. | Code is understandable but has duplication or unclear boundaries. | Code is difficult to trace or changes unrelated state unexpectedly. |
| Debugging practice | Student uses the cycle, tests edge cases, and explains evidence. | Student builds and tests regularly. | Student relies mainly on trial and error. |
| Platform integration | Lifecycle, input, graphics, audio, and shutdown are handled correctly. | Cartridge runs but has minor platform etiquette issues. | Cartridge assumes subsystems always work or leaves state dirty. |
| Publication readiness | Metadata, assets, license, bundle, and store page are complete. | Bundle is mostly complete with small metadata issues. | Publication artifacts are missing or inconsistent. |
| Communication | Student can present design, implementation, and debugging decisions clearly. | Student can describe the game and main code paths. | Student cannot connect behavior to implementation. |

## Common Failure Modes

- **Button repeats too quickly:** Use pressed-edge detection for actions that
  should happen once per press.
- **Pieces appear outside the board:** Separate logical coordinates from pixel
  coordinates and skip drawing blocks with negative `y`.
- **Lines are skipped during clearing:** After shifting rows down, inspect the
  same row index again.
- **Hold can be abused:** Track `hold_used` and reset it only on spawn.
- **Game speed becomes impossible:** Clamp fall frames to a minimum value.
- **Cartridge builds but store bundle fails:** Compare manifest filenames with
  files produced in `dist/`.
- **Game works but device shell looks wrong after exit:** Save platform display
  settings in `init` and restore them in `shutdown`.

## Final Student Checklist

Before submission or publication, each student team should verify:

- The game starts from the title screen.
- All controls listed in the colophon work.
- Pause and restart are reliable.
- Game over occurs when new pieces cannot spawn.
- Score, lines, level, queue, and hold update correctly.
- QEMU and ESP32-C6 builds both complete.
- `assets/icon.png` and `assets/screenshot.png` are present.
- `metadata/metadata.json`, `metadata/manifest.json`, and
  `metadata/colophon.json` are consistent.
- `scripts/pack-store-bundle.sh` creates `dist/debris-store-bundle.zip`.
- The Cartridge Store page is verified after upload.
