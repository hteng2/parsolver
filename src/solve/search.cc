#include "solve/search.h"
#include "db/patterns.h"
#include "lib/turn.h"

#include <cctype>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

Moveset movesetFromString(const std::string &s) {
  Moveset moveset;

  for (char c : s) {
    if (std::isspace(static_cast<uint8_t>(c))) {
      continue;
    }
    std::optional<TurnType> tt = turnTypeFromChar(c);
    if (tt) {
      moveset.types.insert(*tt);
    }
  }

  return moveset;
}

static void nibbleSet(uint8_t *array, int i, uint8_t value) {
  unsigned shift = (i & 1) * 4;
  uint8_t &byte = array[i >> 1];
  uint8_t mask = static_cast<uint8_t>(0xFu << shift);
  uint8_t shifted = static_cast<uint8_t>((value & 0xF) << shift);
  byte = static_cast<uint8_t>((byte & ~mask) | shifted);
}

static Cube solvedCube() {
  Cube cube{};
  for (int i = 0; i < 8; i++) {
    nibbleSet(cube.cornerPerm, i, static_cast<uint8_t>(i));
  }
  for (int i = 0; i < 12; i++) {
    nibbleSet(cube.edgePerm, i, static_cast<uint8_t>(i));
  }
  for (int i = 0; i < 6; i++) {
    nibbleSet(cube.centerPerm, i, static_cast<uint8_t>(i));
  }
  return cube;
}

static bool sameCube(const Cube &a, const Cube &b) {
  for (int i = 0; i < 3; i++) {
    if (a.centerPerm[i] != b.centerPerm[i]) {
      return false;
    }
  }
  for (int i = 0; i < 4; i++) {
    if (a.cornerPerm[i] != b.cornerPerm[i] ||
        a.cornerOri[i] != b.cornerOri[i]) {
      return false;
    }
  }
  for (int i = 0; i < 6; i++) {
    if (a.edgePerm[i] != b.edgePerm[i] || a.edgeOri[i] != b.edgeOri[i]) {
      return false;
    }
  }
  return true;
}

// Face-uniform cubes are exactly the 24 whole-cube rotations of the solved
// cube; precompute them so solvedness needs no facelet conversion.
static const std::vector<Cube> &solvedStates() {
  static const std::vector<Cube> states = [] {
    static const TurnType kGenerators[] = {TurnType::X, TurnType::Y};
    std::vector<Cube> result;
    result.push_back(solvedCube());

    for (size_t i = 0; i < result.size(); i++) {
      for (TurnType tt : kGenerators) {
        Cube next = result[i];
        Turn turn{1, tt};
        doTurn(next, turn);

        bool seen = false;
        for (const Cube &known : result) {
          if (sameCube(known, next)) {
            seen = true;
            break;
          }
        }
        if (!seen) {
          result.push_back(next);
        }
      }
    }

    return result;
  }();
  return states;
}

static bool isSolved(const Cube &cube) {
  for (const Cube &solved : solvedStates()) {
    if (sameCube(cube, solved)) {
      return true;
    }
  }
  return false;
}

struct AxisLayers {
  int axis;       // 0 = x, 1 = y, 2 = z
  uint8_t layers; // bit 0: -1, bit 1: 0, bit 2: +1
};

static AxisLayers moveAxis(TurnType tt) {
  switch (tt) {
  case TurnType::U:
    return {1, 0b100};
  case TurnType::UW:
    return {1, 0b110};
  case TurnType::E:
    return {1, 0b010};
  case TurnType::D:
    return {1, 0b001};
  case TurnType::DW:
    return {1, 0b011};

  case TurnType::R:
    return {0, 0b100};
  case TurnType::RW:
    return {0, 0b110};
  case TurnType::M:
    return {0, 0b010};
  case TurnType::L:
    return {0, 0b001};
  case TurnType::LW:
    return {0, 0b011};

  case TurnType::F:
    return {2, 0b100};
  case TurnType::FW:
    return {2, 0b110};
  case TurnType::S:
    return {2, 0b010};
  case TurnType::B:
    return {2, 0b001};
  case TurnType::BW:
    return {2, 0b011};

  case TurnType::X:
    return {0, 0b111};
  case TurnType::Y:
    return {1, 0b111};
  case TurnType::Z:
    return {2, 0b111};
  }
  return {-1, 0};
}

// Moves on the same axis with disjoint layers commute, so only allow one of
// their two orderings: higher layers first (U before D, F before B, R before
// L).
static bool incompatible(TurnType prev, TurnType next) {
  if (prev == next) {
    return true;
  }

  AxisLayers a = moveAxis(prev);
  AxisLayers b = moveAxis(next);
  if (a.axis != b.axis || (a.layers & b.layers)) {
    return false;
  }

  return b.layers > a.layers;
}

static void search(Cube &cube, const CornerDb *db, const Moveset &moveset,
                   uint32_t threshold, uint32_t g, std::optional<TurnType> last,
                   std::vector<Turn> &path) {
  if (isSolved(cube) && g == threshold) {
    std::cout << "solution:";
    for (const Turn &turn : path) {
      std::cout << ' ' << turnToString(turn);
    }
    std::cout << '\n';
  }

  if (g == threshold) {
    return;
  }

  for (TurnType tt : moveset.types) {
    if (last && incompatible(*last, tt)) {
      continue;
    }

    for (uint8_t amt = 1; amt <= 3; amt++) {
      Turn move{amt, tt};
      Cube next = cube;
      doTurn(next, move);

      uint32_t h = db ? cornerHeuristic(next, *db) : 0;
      if (g + 1 + h > threshold) {
        continue;
      }

      path.push_back(move);
      search(next, db, moveset, threshold, g + 1, tt, path);
      path.pop_back();
    }
  }
}

void solve(Cube &c, uint8_t length, Moveset moveset, const CornerDb *db) {
  uint32_t start = db ? cornerHeuristic(c, *db) : 0;
  if (start > length) {
    return;
  }

  std::vector<Turn> path;
  for (uint32_t threshold = start; threshold <= length; threshold++) {
    std::cout << "depth: " << threshold << '\n';
    search(c, db, moveset, threshold, 0, std::nullopt, path);
  }
}
