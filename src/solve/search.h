#ifndef search_h
#define search_h

#include "db/patterns.h"
#include "lib/turn.h"
#include <cstdint>
#include <string>
#include <unordered_set>

struct Moveset {
  std::unordered_set<TurnType> types;
};

Moveset movesetFromString(const std::string &s);

// Prints solutions of length at most `length` and stops after `maxSolutions` of
// them (0 means no limit).
void solve(Cube &c, uint8_t length, Moveset moveset, const CornerDb *db,
           uint64_t maxSolutions);

#endif
