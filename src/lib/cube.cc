#include "lib/cube.h"

Color colorFromChar(char c) {
  switch (c) {
  case 'w':
    return Color::White;
  case 'y':
    return Color::Yellow;
  case 'g':
    return Color::Green;
  case 'b':
    return Color::Blue;
  case 'r':
    return Color::Red;
  case 'o':
    return Color::Orange;
  case 'd':
    return Color::Gray;
  default:
    return Color::None;
  }
}

static void setNibble(uint8_t *array, int i, uint8_t value) {
  unsigned shift = (i & 1) * 4;
  uint8_t &byte = array[i >> 1];
  uint8_t mask = static_cast<uint8_t>(0xFu << shift);
  uint8_t shifted = static_cast<uint8_t>((value & 0xF) << shift);
  byte = static_cast<uint8_t>((byte & ~mask) | shifted);
}

Cube cubeDefault() {
  Cube cube{};

  for (int i = 0; i < 8; i++) {
    setNibble(cube.cornerPerm, i, static_cast<uint8_t>(i));
  }
  for (int i = 0; i < 12; i++) {
    setNibble(cube.edgePerm, i, static_cast<uint8_t>(i));
  }
  for (int i = 0; i < 6; i++) {
    setNibble(cube.centerPerm, i, static_cast<uint8_t>(i));
  }

  // Orientations are already zero: every piece is in its solved orientation.
  return cube;
}
