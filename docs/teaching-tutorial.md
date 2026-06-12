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

