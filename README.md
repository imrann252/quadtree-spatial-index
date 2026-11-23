# Boids Part III — Finding Neighbors with a Quadtree

## What to edit

| File | What you write |
|---|---|
| `src/quadtree.c` (bottom half) | Tasks 1–4: `qtCreate`, `qtFree`, `split`, `qtInsert`, `qtQuery`, `qtHeight`, `qtCountNodes`. Task 6: `qtFindLeaf`, `merge`, `qtRemove`. Task 7: `qtNearest` |
| `src/boids.c` (under the "TASK 5" and "TASK 6" banners) | Task 5: `buildTree`, `findNeighborsQuadtree`. Task 6: `updateTree` |

Every function you write has a comment above it saying what it must do.
Everything else is provided: read it, but do not change it.

## Files

| File | What it is |
|---|---|
| `src/boid.h` | The `Boid` struct from Part I |
| `src/dynarray.h` | The `DynamicArray` from Part II |
| `src/quadtree.h`, `src/quadtree.c` | The quadtree: provided helpers (top) and your Tasks 1–4, 6 and 7 (bottom) |
| `src/boids.h`, `src/boids.c` | The flocking simulation, brute-force search, and your Tasks 5 and 6 |
| `src/main.c` | The window (raylib) |
| `src/bench.c`, `src/timer.h` | Timing without a window, for Task 8 |
| `tests/` | Automatic checks for Tasks 1–7, including memory-leak checks |
| `raylib/` | raylib 5.5, built automatically the first time you run `make` |

## Setup (once)

raylib is already included in the `raylib/` folder: you do **not** install it.
You only need a C compiler (gcc) and `make`.

- **Windows:** download the x64 file of the latest release of **w64devkit**
  from https://github.com/skeeto/w64devkit/releases (its name looks like
  `w64devkit-x64-2.x.x.exe`). Run it: it unpacks itself into a folder you
  choose, with no installation and no administrator rights. Then start
  `w64devkit.exe` inside that folder. It opens a terminal that has `gcc` and
  `make`; always work in this window.
- **macOS:** open Terminal and run `xcode-select --install`, then click Install.
- **Linux (Ubuntu/Debian):**
  `sudo apt install build-essential libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`
  (Fedora: `sudo dnf install gcc make libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel`)

Check it worked: `gcc --version` and `make --version` should each print a
version number.

## First build

In the terminal, go into this folder (type `cd `, drag the folder into the
window, press Enter), then:

```
make        # the first time this also builds raylib: about a minute
./boids     # opens the window
make test   # every check fails until you write the functions; that is expected
```

On Windows, `./boids.exe` and `./bench.exe` also work.

## The window

- **M** switches the method: `brute` → `quadtree` → `update` (Task 6).
  A method whose code is not written yet falls back to brute force with a message.
- **T** shows the quadtree's boxes.
- **R** restarts with a new flock.

## Commands

```
make test               # run all checks (they also find memory leaks)
./run_tests 6           # run only the checks for Task 6
make bench              # build the timing program (Task 8)
./bench --method brute --n 1000      # time brute force with 1,000 boids
./bench --method quadtree --n 1000   # a new quadtree every frame (Tasks 1-5)
./bench --method update --n 1000     # one kept quadtree, updated (Task 6)
./bench --method quadtree --n 10000 --nearest 5000
                                     # also time 5,000 nearest-boid searches (Task 7)
./bench --method quadtree --n 1000 --spawn same-point --frames 0
                                     # all boids on one spot: tree shape only
make clean              # delete everything that was built
```

Timing options: `--method` is required (`brute`, `quadtree` or `update`). The others are optional: `--n` number of boids (default 1000),
`--frames` frames to time (default 200), `--seed` starting positions
(default 42), `--spawn same-point` puts every boid on one spot, `--nearest`
number of nearest-boid searches to time.

`make test` and `make bench` do not use raylib at all, so you can do every
task even if your window does not work yet. On Linux and macOS, `make asan`
adds extra memory checking with AddressSanitizer (optional).

## If something goes wrong

- "make: command not found" or "gcc: command not found": on Windows you are
  not in the w64devkit window; on macOS the installation did not finish; on
  Linux the packages are not installed.
- "No rule to make target" or "No such file or directory": you are not in this
  folder. Type `ls` and check that you see the Makefile.
- On Linux, an error that mentions X11 or a file such as `Xrandr.h`: one of
  the packages above is missing.

## Suggested order

Do the tasks in order and run `./run_tests N` after each one. Each task's
checks only use the tasks before it.
