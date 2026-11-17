#ifndef turn_h
#define turn_h

#include "lib/cube.h"
#include <cstdint>
#include <optional>
#include <string>

enum class TurnType : uint8_t {
  U,
  D,
  F,
  B,
  R,
  L,
  UW,
  DW,
  FW,
  BW,
  RW,
  LW,
  X,
  Y,
  Z,
  M,
  E,
  S,
};

struct Turn {
  uint8_t amt : 3; // number of clockwise quarter turns
  TurnType tt : 5;
};

std::optional<TurnType> turnTypeFromChar(char c);
std::optional<Turn> turnFromString(const std::string &s);
std::string turnToString(const Turn &t);
void doTurn(Cube &c, Turn &t);

#endif
