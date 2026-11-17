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

| Flag | Required | Meaning |
|---|---|---|
| `-m <moves>` | yes | Space-separated scramble applied to a solved cube. May be `""`. |
| `-s <moveset>` | yes | Allowed move types for the search. |
| `-l <length>` | yes | Maximum solution length, `0`–`32`. |
| `-p <db>` | no | Corner pattern database file to load. |

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

- Progress is printed every 100,000 visited states (`depth`, states explored,
  queue size), followed by a `done: … states, max depth …` line.
- The table is a fixed **132,269,760-byte** packed nibble array.
- It is built over the six face turns; wide turns have the same effect on
  corners, slices do not touch corners, and rotations are symmetries of the
  rotation-invariant goal set.
- The build visits all 88,179,840 corner states and can take several minutes.
  Run it once and reuse the file with `solve -p`.

> Database files are tied to the current corner-orientation encoding. There is
> no version header, so tables produced by an older build of `parsolver` must be
> regenerated.

## Move notation

Moves are a single move type optionally followed by `2` (half turn) or `'`
(counterclockwise). Move types:

| Notation | Move |
|---|---|
| `U D F B R L` | face turns, clockwise as seen from outside that face |
| `u d f b r l` | wide turns (the face plus the adjacent middle slice) |
| `x y z` | whole-cube rotations |
| `M E S` | middle-slice turns |

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
