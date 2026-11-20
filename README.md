# parsolver

A 3x3x3 Rubik's cube solver with a corner pattern database and IDA* search.

## Build

```sh
make
```

This produces `build/parsolver`. The build uses `clang++` with `-Wall -Wextra
-O3 -Isrc`, discovers sources under `src/` automatically, and emits header
dependencies (`-MMD -MP`).

## Usage

```
parsolver solve -m <moves> -s <moveset> -l <length> [-p <db>]
parsolver build-db <path>
```

### `solve`

Applies a scramble, prints the resulting cube, then runs IDA* and prints the
solution(s).

| Flag           | Required | Meaning                                                         |
| -------------- | -------- | --------------------------------------------------------------- |
| `-m <moves>`   | yes      | Space-separated scramble applied to a solved cube. May be `""`. |
| `-s <moveset>` | yes      | Allowed move types for the search.                              |
| `-l <length>`  | yes      | Maximum solution length, `0`–`32`.                              |
| `-p <db>`      | no       | Corner pattern database file to load.                           |

- With `-p`, the search uses the corner database as its heuristic.
- Without `-p`, no database is built or written; the heuristic is `0`, so the
  search is an uninformed iterative-deepening DFS bounded by `-l`.
- An unreadable or wrong-sized `-p` file is an error (exit `1`).
- `-m`, `-s`, and `-l` are mandatory; unknown flags and values outside the
  allowed ranges exit `1`.

### `build-db`

```
parsolver build-db <path>
```

Builds the complete corner pattern database and writes it to `<path>`.

- The table is a fixed **44,089,920-byte** packed nibble array: one nibble per
  reachable corner state (`Lehmer` permutation code × base-3 orientation, with
  DBL's orientation implied by the twist invariant).
- It is built over `U`, `F` and `R` only: `D·U'`, `B·F'` and `L·R'` are
  whole-cube rotations, and a rotation does not change a corner distance, so the
  other three faces are redundant. Wide turns have the same effect on corners
  as their face, slices do not touch corners, and rotations are symmetries of
  the rotation-invariant goal set.
- The build visits all 88,179,840 corner states (about 22 s on the reference
  machine below). Run it once and reuse the file with `solve -p`.

> Database files are tied to the current corner-orientation encoding. There is
> no version header, so tables produced by an older build of `parsolver` must be
> regenerated.

## Database build performance

`build-db` is the only expensive operation. Reference machine: Intel Core Ultra
9 185H (22 logical CPUs), Linux 7.3.4, `clang++ -O3`, single-threaded, database
written to disk on every run.

Two versions are compared: **previous**, commit `dfffbb2` (six face-turn
generators, 3⁸ key space, 132,269,760-byte table) and **current**, this tree
(three generators, 3⁷ key space, 44,089,920-byte table). The previous tree is
built from `git archive dfffbb2` with the same `Makefile`. Ten independent
samples per version, run interleaved, each timed with
`/usr/bin/time -f "%e %U %S"`.

| Run | previous elapsed | previous user | previous sys | current elapsed | current user | current sys |
| --: | ---------------: | ------------: | -----------: | --------------: | -----------: | ----------: |
|   1 |           100.87 |        100.33 |         0.15 |           24.61 |        24.45 |        0.06 |
|   2 |           103.49 |        102.76 |         0.15 |           24.20 |        23.99 |        0.08 |
|   3 |           100.73 |        100.09 |         0.15 |           23.78 |        23.55 |        0.08 |
|   4 |            98.89 |         98.38 |         0.12 |           22.28 |        22.03 |        0.11 |
|   5 |            98.16 |         97.54 |         0.14 |           20.81 |        20.62 |        0.09 |
|   6 |            93.56 |         93.15 |         0.13 |           20.37 |        20.20 |        0.08 |
|   7 |            94.15 |         93.71 |         0.12 |           21.29 |        21.09 |        0.08 |
|   8 |           109.16 |        103.15 |         0.15 |           21.15 |        20.92 |        0.09 |
|   9 |            96.22 |         95.60 |         0.16 |           21.26 |        21.05 |        0.10 |
|  10 |            93.36 |         92.90 |         0.15 |           20.89 |        20.69 |        0.10 |

All times in seconds. Standard deviation is the sample SD (`n - 1`).

| Version  | Metric  |  Mean | Median |   Min |    Max |    SD |
| -------- | ------- | ----: | -----: | ----: | -----: | ----: |
| previous | elapsed | 98.86 |  98.53 | 93.36 | 109.16 |  4.98 |
| previous | user    | 97.76 |  97.96 | 92.90 | 103.15 |  3.84 |
| previous | sys     | 0.142 |  0.150 | 0.120 |  0.160 | 0.014 |
| current  | elapsed | 22.06 |  21.28 | 20.37 |  24.61 |  1.56 |
| current  | user    | 21.86 |  21.07 | 20.20 |  24.45 |  1.56 |
| current  | sys     | 0.087 |  0.085 | 0.060 |  0.110 | 0.014 |

Paired elapsed speedup (previous / current, per run): mean **4.49×**, median
**4.45×**, range 4.10×–5.16×; mean 76.8 s and median 76.4 s saved per build.

Elapsed tracks user time closely (sys stays under 0.2 s), so the gain is compute
rather than I/O. The two changes contribute roughly equally:

- **Generator reduction.** The BFS applies 9 quarter turns per state instead of
  18, cutting the expansions from 1.59e9 to 7.94e8 (2.0×).
- **Smaller table.** CPU time per expansion falls from 61.6 ns to 27.5 ns (2.2×),
  consistent with a 3× smaller array: fewer page faults and fewer cache misses
  on the random `db.get`/`db.set` accesses.

2.0 × 2.2 ≈ 4.5, which matches the measured speedup. The previous-version spread
(93–109 s) is machine interference: run 8 spent ~6 s of elapsed time outside the
process.

## Move notation

Moves are a single move type optionally followed by `2` (half turn) or `'`
(counterclockwise). Move types:

| Notation      | Move                                                 |
| ------------- | ---------------------------------------------------- |
| `U D F B R L` | face turns, clockwise as seen from outside that face |
| `u d f b r l` | wide turns (the face plus the adjacent middle slice) |
| `x y z`       | whole-cube rotations                                 |
| `M E S`       | middle-slice turns                                   |

Examples: `U`, `R2`, `F'`, `r'`, `x`, `M2`, `S'`.

The scramble (`-m`) accepts full moves, e.g. `"R U R' U'"`. The moveset (`-s`)
accepts types (spaces are optional), e.g. `"UDFBRL"`, `"UD"`, `"rU"`, or
`"U D F B R L"`; characters that are not a move type are ignored.

## Output

The cube is printed as a net using `w y g b r o` (white, yellow, green, blue,
red, orange) with ANSI colors:

```
      w w w
      w w w
      w w w
o o o g g g r r r b b b
o o o g g g r r r b b b
o o o g g g r r r b b b
      y y y
      y y y
      y y y
```

The search prints one `depth: N` line per IDA* bound, followed by
`solution: <moves>` for every solution found at that bound. Solutions use the
same notation as the input. Any whole-cube rotation of the solved cube is
considered solved.

## Examples

Solve a scramble without a database:

```sh
./build/parsolver solve -m "R U R' U'" -s "UDFBRL" -l 4
```

```
...
depth: 4
solution: U R U' R'
```

Build a database once and reuse it:

```sh
./build/parsolver build-db pdb
./build/parsolver solve -p pdb -m "R U F L" -s "UDFBRL" -l 4
```

Restrict the search to a subset of moves:

```sh
./build/parsolver solve -m "r U" -s "rU" -l 2
```

## Project layout

```
src/
  main.cc           CLI and subcommands
  db/patterns.*     corner pattern database
  solve/search.*    IDA* search
  lib/cube.*        cubie cube representation
  lib/turn.*        move parsing and application
  utils/display.*   cubie-to-net rendering
```
