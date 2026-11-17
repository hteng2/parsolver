#ifndef search_h
#define search_h

#include "db/patterns.h"
#include "lib/turn.h"
#include <string>
#include <unordered_set>

struct Moveset {
  std::unordered_set<TurnType> types;
};

Moveset movesetFromString(const std::string &s);

void solve(Cube &c, uint8_t length, Moveset moveset, const CornerDb *db);

#endif
