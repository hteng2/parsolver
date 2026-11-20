#include "db/patterns.h"
#include "lib/cube.h"
#include "lib/turn.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <queue>
#include <string>
#include <vector>

// The database key is built directly from the cube's corner arrays:
//   key = Lehmer(cornerPerm) * 3^7 + base-3(cornerOri of the non-DBL corners)
// No facelet-to-cubie conversion is needed.

// Every corner can be solved in fewer than 16 moves, so distances fit in a
// nibble. Two states share each byte of the packed table.
static const uint8_t kUnvisited = 0xF;
static const uint32_t kOrientationCount = 2187u;  // 3^7
static const uint32_t kPermutationCount = 40320u; // 8!
static const uint32_t kCornerKeyCount = kPermutationCount * kOrientationCount;

CornerDb::CornerDb() : data((kCornerKeyCount + 1) / 2, 0xFF) {}

uint8_t CornerDb::get(uint32_t key) const {
  uint8_t byte = data[key >> 1];
  return (key & 1) ? static_cast<uint8_t>(byte >> 4)
                   : static_cast<uint8_t>(byte & 0xF);
}

void CornerDb::set(uint32_t key, uint8_t dist) {
  uint8_t &byte = data[key >> 1];
  if (key & 1) {
    byte = static_cast<uint8_t>((byte & 0x0F) | (dist << 4));
  } else {
    byte = static_cast<uint8_t>((byte & 0xF0) | dist);
  }
}

static uint8_t nibbleGet(const uint8_t *array, int i) {
  return (array[i >> 1] >> ((i & 1) * 4)) & 0xF;
}

static void nibbleSet(uint8_t *array, int i, uint8_t value) {
  unsigned shift = (i & 1) * 4;
  uint8_t &byte = array[i >> 1];
  uint8_t mask = static_cast<uint8_t>(0xFu << shift);
  uint8_t shifted = static_cast<uint8_t>((value & 0xF) << shift);
  byte = static_cast<uint8_t>((byte & ~mask) | shifted);
}

// Lehmer code of the corner permutation (position -> piece).
static uint32_t permRank(const Cube &cube) {
  uint32_t rank = 0;
  for (int i = 0; i < 8; i++) {
    uint32_t piece = nibbleGet(cube.cornerPerm, i);
    uint32_t smaller = 0;
    for (int j = i + 1; j < 8; j++) {
      if (nibbleGet(cube.cornerPerm, j) < piece) {
        smaller++;
      }
    }
    rank = rank * static_cast<uint32_t>(8 - i) + smaller;
  }
  return rank;
}

// Corner orientations always sum to 0 mod 3, so the orientation of DBL
// (position 6) is implied by the other seven and is left out. That shrinks the
// key space to the 8! * 3^7 corner states that can actually occur.
static uint32_t oriCode(const Cube &cube) {
  uint32_t code = 0;
  uint32_t place = 1;
  for (int i = 0; i < 8; i++) {
    if (i == 6) {
      continue;
    }
    code += nibbleGet(cube.cornerOri, i) * place;
    place *= 3;
  }
  return code;
}

static uint32_t cornerKey(const Cube &cube) {
  return permRank(cube) * kOrientationCount + oriCode(cube);
}

// Reconstructs a cube carrying the decoded corner state. Edges and centers are
// irrelevant to corner turns, so they are left as the solved identity.
static Cube cubeFromKey(uint32_t key) {
  Cube cube{};
  for (int i = 0; i < 12; i++) {
    nibbleSet(cube.edgePerm, i, static_cast<uint8_t>(i));
  }
  for (int i = 0; i < 6; i++) {
    nibbleSet(cube.centerPerm, i, static_cast<uint8_t>(i));
  }

  uint32_t ori = key % kOrientationCount;
  uint32_t rank = key / kOrientationCount;
  uint32_t twist = 0;
  for (int i = 0; i < 8; i++) {
    if (i == 6) {
      continue;
    }
    uint8_t o = static_cast<uint8_t>(ori % 3);
    nibbleSet(cube.cornerOri, i, o);
    twist += o;
    ori /= 3;
  }
  nibbleSet(cube.cornerOri, 6, static_cast<uint8_t>((3 - twist % 3) % 3));

  uint8_t counts[8];
  for (int i = 7; i >= 0; i--) {
    uint32_t divisor = static_cast<uint32_t>(8 - i);
    counts[i] = static_cast<uint8_t>(rank % divisor);
    rank /= divisor;
  }

  bool used[8] = {false, false, false, false, false, false, false, false};
  for (int i = 0; i < 8; i++) {
    uint32_t pick = counts[i];
    for (int piece = 0; piece < 8; piece++) {
      if (used[piece]) {
        continue;
      }
      if (pick == 0) {
        nibbleSet(cube.cornerPerm, i, static_cast<uint8_t>(piece));
        used[piece] = true;
        break;
      }
      pick--;
    }
  }

  return cube;
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

// Corner states only need three generators: D·U', B·F' and L·R' are whole-cube
// rotations, so any word over the six face turns has an equally long word over
// {U, F, R} that differs from it by rotations, and a rotation does not change a
// corner distance. Wide turns have the same corner effect as their face, and
// slices do not touch corners.
static const TurnType kCornerTurns[] = {
    TurnType::U,
    TurnType::F,
    TurnType::R,
};

// Whole-cube rotations of the solved cube are also solved states.
static const TurnType kRotations[] = {TurnType::X, TurnType::Y};

CornerDb buildCornerDb() {
  CornerDb db;
  std::queue<uint32_t> queue;

  // record all rotations of the solved cube
  Cube solved = solvedCube();

  std::queue<Cube> rotations;
  rotations.push(solved);
  uint32_t solvedKey = cornerKey(solved);
  db.set(solvedKey, 0);
  queue.push(solvedKey);

  while (!rotations.empty()) {
    Cube st = rotations.front();
    rotations.pop();
    for (TurnType tt : kRotations) {
      Cube next = st;
      Turn turn{1, tt};
      doTurn(next, turn);
      uint32_t key = cornerKey(next);
      if (db.get(key) == kUnvisited) {
        db.set(key, 0);
        rotations.push(next);
        queue.push(key);
      }
    }
  }

  // BFS visit of all remaining states
  while (!queue.empty()) {
    uint32_t key = queue.front();
    queue.pop();
    uint8_t dist = db.get(key);

    Cube st = cubeFromKey(key);
    for (TurnType tt : kCornerTurns) {
      Turn turn{1, tt};
      for (uint8_t amt = 1; amt <= 3; amt++) {
        doTurn(st, turn);
        uint32_t nextKey = cornerKey(st);
        if (db.get(nextKey) == kUnvisited) {
          db.set(nextKey, static_cast<uint8_t>(dist + 1));
          queue.push(nextKey);
        }
      }
      doTurn(st, turn);
    }
  }

  return db;
}

uint32_t cornerHeuristic(const Cube &cube, const CornerDb &db) {
  uint8_t dist = db.get(cornerKey(cube));
  if (dist == kUnvisited) {
    return 255;
  }
  return dist;
}

bool saveCornerDb(const CornerDb &db, const std::string &path) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    return false;
  }
  out.write(reinterpret_cast<const char *>(db.data.data()),
            static_cast<std::streamsize>(db.data.size()));
  return static_cast<bool>(out);
}

std::optional<CornerDb> loadCornerDb(const std::string &path) {
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  if (!in) {
    return std::nullopt;
  }

  std::streamsize size = in.tellg();
  if (size < 0 || static_cast<std::size_t>(size) != (kCornerKeyCount + 1) / 2) {
    return std::nullopt;
  }

  CornerDb db;
  in.seekg(0);
  in.read(reinterpret_cast<char *>(db.data.data()), size);
  if (!in) {
    return std::nullopt;
  }
  return db;
}
