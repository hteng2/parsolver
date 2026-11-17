#include "lib/turn.h"
#include "lib/cube.h"

#include <cstdint>
#include <optional>

std::optional<TurnType> turnTypeFromChar(char c) {
  switch (c) {
  case 'U':
    return TurnType::U;
  case 'D':
    return TurnType::D;
  case 'F':
    return TurnType::F;
  case 'B':
    return TurnType::B;
  case 'R':
    return TurnType::R;
  case 'L':
    return TurnType::L;

  case 'u':
    return TurnType::UW;
  case 'd':
    return TurnType::DW;
  case 'f':
    return TurnType::FW;
  case 'b':
    return TurnType::BW;
  case 'r':
    return TurnType::RW;
  case 'l':
    return TurnType::LW;

  case 'x':
    return TurnType::X;
  case 'y':
    return TurnType::Y;
  case 'z':
    return TurnType::Z;

  case 'M':
    return TurnType::M;
  case 'E':
    return TurnType::E;
  case 'S':
    return TurnType::S;

  default:
    return std::nullopt;
  }
}

std::optional<Turn> turnFromString(const std::string &s) {
  if (s.length() == 0 || s.length() > 2) {
    return std::nullopt;
  }

  uint8_t amt;
  if (s.back() == '\'') {
    amt = 3;
  } else if (s.back() == '2') {
    amt = 2;
  } else if (s.length() == 1) {
    amt = 1;
  } else {
    return std::nullopt;
  }

  std::optional<TurnType> tt = turnTypeFromChar(s[0]);
  if (!tt) {
    return std::nullopt;
  }

  return Turn{amt, *tt};
}

std::string turnToString(const Turn &t) {
  char c;
  switch (t.tt) {
  case TurnType::U:
    c = 'U';
    break;
  case TurnType::D:
    c = 'D';
    break;
  case TurnType::F:
    c = 'F';
    break;
  case TurnType::B:
    c = 'B';
    break;
  case TurnType::R:
    c = 'R';
    break;
  case TurnType::L:
    c = 'L';
    break;
  case TurnType::UW:
    c = 'u';
    break;
  case TurnType::DW:
    c = 'd';
    break;
  case TurnType::FW:
    c = 'f';
    break;
  case TurnType::BW:
    c = 'b';
    break;
  case TurnType::RW:
    c = 'r';
    break;
  case TurnType::LW:
    c = 'l';
    break;
  case TurnType::X:
    c = 'x';
    break;
  case TurnType::Y:
    c = 'y';
    break;
  case TurnType::Z:
    c = 'z';
    break;
  case TurnType::M:
    c = 'M';
    break;
  case TurnType::E:
    c = 'E';
    break;
  case TurnType::S:
    c = 'S';
    break;
  }

  std::string s(1, c);
  if (t.amt == 2) {
    s += '2';
  } else if (t.amt == 3) {
    s += '\'';
  }
  return s;
}

// Nibble accessors for the packed cubie arrays.
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

static int mod3(int value) {
  value %= 3;
  return value < 0 ? value + 3 : value;
}

/**
 * Cycle corner pieces c1 -> c2 -> c3 -> c4 -> c1.
 * Pieces taken from c1 and c3 are twisted by `twist`, the ones from c2 and c4
 * by the opposite amount (mod 3). For example R twists UFR clockwise and DFR
 * counterclockwise, while U twists nothing.
 */
static void cycleCorners(Cube &cube, int c1, int c2, int c3, int c4,
                         int twist) {
  uint8_t p1 = nibbleGet(cube.cornerPerm, c1);
  uint8_t p2 = nibbleGet(cube.cornerPerm, c2);
  uint8_t p3 = nibbleGet(cube.cornerPerm, c3);
  uint8_t p4 = nibbleGet(cube.cornerPerm, c4);
  int o1 = nibbleGet(cube.cornerOri, c1);
  int o2 = nibbleGet(cube.cornerOri, c2);
  int o3 = nibbleGet(cube.cornerOri, c3);
  int o4 = nibbleGet(cube.cornerOri, c4);

  nibbleSet(cube.cornerPerm, c2, p1);
  nibbleSet(cube.cornerOri, c2, static_cast<uint8_t>(mod3(o1 + twist)));
  nibbleSet(cube.cornerPerm, c3, p2);
  nibbleSet(cube.cornerOri, c3, static_cast<uint8_t>(mod3(o2 - twist)));
  nibbleSet(cube.cornerPerm, c4, p3);
  nibbleSet(cube.cornerOri, c4, static_cast<uint8_t>(mod3(o3 + twist)));
  nibbleSet(cube.cornerPerm, c1, p4);
  nibbleSet(cube.cornerOri, c1, static_cast<uint8_t>(mod3(o4 - twist)));
}

/**
 * Cycle edge pieces e1 -> e2 -> e3 -> e4 -> e1, flipping all four when `flip`.
 * F flips every edge on F, while R flips nothing (ZZ-style orientation).
 */
static void cycleEdges(Cube &cube, int e1, int e2, int e3, int e4, bool flip) {
  uint8_t p1 = nibbleGet(cube.edgePerm, e1);
  uint8_t p2 = nibbleGet(cube.edgePerm, e2);
  uint8_t p3 = nibbleGet(cube.edgePerm, e3);
  uint8_t p4 = nibbleGet(cube.edgePerm, e4);
  uint8_t o1 = nibbleGet(cube.edgeOri, e1);
  uint8_t o2 = nibbleGet(cube.edgeOri, e2);
  uint8_t o3 = nibbleGet(cube.edgeOri, e3);
  uint8_t o4 = nibbleGet(cube.edgeOri, e4);
  uint8_t f = flip ? 1 : 0;

  nibbleSet(cube.edgePerm, e2, p1);
  nibbleSet(cube.edgeOri, e2, o1 ^ f);
  nibbleSet(cube.edgePerm, e3, p2);
  nibbleSet(cube.edgeOri, e3, o2 ^ f);
  nibbleSet(cube.edgePerm, e4, p3);
  nibbleSet(cube.edgeOri, e4, o3 ^ f);
  nibbleSet(cube.edgePerm, e1, p4);
  nibbleSet(cube.edgeOri, e1, o4 ^ f);
}

/**
 * Cycle centers c1 -> c2 -> c3 -> c4 -> c1. Centers only have a permutation;
 * orientation is defined relative to the fixed UD/FB axes, not the centers.
 */
static void cycleCenters(Cube &cube, int c1, int c2, int c3, int c4) {
  uint8_t p1 = nibbleGet(cube.centerPerm, c1);
  uint8_t p2 = nibbleGet(cube.centerPerm, c2);
  uint8_t p3 = nibbleGet(cube.centerPerm, c3);
  uint8_t p4 = nibbleGet(cube.centerPerm, c4);

  nibbleSet(cube.centerPerm, c2, p1);
  nibbleSet(cube.centerPerm, c3, p2);
  nibbleSet(cube.centerPerm, c4, p3);
  nibbleSet(cube.centerPerm, c1, p4);
}

// Each turn below is one clockwise quarter turn. Wide turns and whole-cube
// rotations are composed from the face and slice turns.

static void turnU(Cube &cube) {
  cycleCorners(cube, 0, 3, 2, 1, 0);
  cycleEdges(cube, 0, 3, 2, 1, false);
}

static void turnD(Cube &cube) {
  cycleCorners(cube, 4, 5, 6, 7, 0);
  cycleEdges(cube, 8, 9, 10, 11, false);
}

static void turnF(Cube &cube) {
  cycleCorners(cube, 0, 4, 7, 3, 2);
  cycleEdges(cube, 0, 4, 8, 5, true);
}

static void turnB(Cube &cube) {
  cycleCorners(cube, 1, 2, 6, 5, 1);
  cycleEdges(cube, 2, 7, 10, 6, true);
}

static void turnR(Cube &cube) {
  cycleCorners(cube, 0, 1, 5, 4, 1);
  cycleEdges(cube, 1, 6, 9, 4, false);
}

static void turnL(Cube &cube) {
  cycleCorners(cube, 2, 3, 7, 6, 1);
  cycleEdges(cube, 3, 5, 11, 7, false);
}

static void turnM(Cube &cube) {
  cycleEdges(cube, 0, 8, 10, 2, true);
  cycleCenters(cube, 0, 2, 1, 3);
}

static void turnE(Cube &cube) {
  cycleEdges(cube, 4, 6, 7, 5, true);
  cycleCenters(cube, 2, 4, 3, 5);
}

static void turnS(Cube &cube) {
  cycleEdges(cube, 1, 9, 11, 3, true);
  cycleCenters(cube, 0, 4, 1, 5);
}

static void turnUW(Cube &cube) {
  turnU(cube);
  turnE(cube);
  turnE(cube);
  turnE(cube);
}

static void turnDW(Cube &cube) {
  turnD(cube);
  turnE(cube);
}

static void turnFW(Cube &cube) {
  turnF(cube);
  turnS(cube);
}

static void turnBW(Cube &cube) {
  turnB(cube);
  turnS(cube);
  turnS(cube);
  turnS(cube);
}

static void turnRW(Cube &cube) {
  turnR(cube);
  turnM(cube);
  turnM(cube);
  turnM(cube);
}

static void turnLW(Cube &cube) {
  turnL(cube);
  turnM(cube);
}

static void turnX(Cube &cube) {
  turnRW(cube);
  turnL(cube);
  turnL(cube);
  turnL(cube);
}

static void turnY(Cube &cube) {
  turnUW(cube);
  turnD(cube);
  turnD(cube);
  turnD(cube);
}

static void turnZ(Cube &cube) {
  turnFW(cube);
  turnB(cube);
  turnB(cube);
  turnB(cube);
}

void doTurn(Cube &c, Turn &t) {
  void (*turn)(Cube &);
  switch (t.tt) {
  case TurnType::U:
    turn = turnU;
    break;
  case TurnType::D:
    turn = turnD;
    break;
  case TurnType::F:
    turn = turnF;
    break;
  case TurnType::B:
    turn = turnB;
    break;
  case TurnType::R:
    turn = turnR;
    break;
  case TurnType::L:
    turn = turnL;
    break;
  case TurnType::UW:
    turn = turnUW;
    break;
  case TurnType::DW:
    turn = turnDW;
    break;
  case TurnType::FW:
    turn = turnFW;
    break;
  case TurnType::BW:
    turn = turnBW;
    break;
  case TurnType::RW:
    turn = turnRW;
    break;
  case TurnType::LW:
    turn = turnLW;
    break;
  case TurnType::X:
    turn = turnX;
    break;
  case TurnType::Y:
    turn = turnY;
    break;
  case TurnType::Z:
    turn = turnZ;
    break;
  case TurnType::M:
    turn = turnM;
    break;
  case TurnType::E:
    turn = turnE;
    break;
  case TurnType::S:
    turn = turnS;
    break;
  default:
    return;
  }

  for (uint8_t i = 0; i < t.amt; i++) {
    turn(c);
  }
}
