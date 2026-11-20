#ifndef patterns_h
#define patterns_h

#include "lib/cube.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Packed corner pattern database. A key is the corner state encoded as
//   Lehmer(perm) * 3^7 + base-3 orientation of the seven non-DBL corners
// where perm[i] is the id of the corner piece at position i (see patterns.cc).
// DBL's orientation is implied by the others (they always sum to 0 mod 3), so
// the table has one entry per reachable corner state: 8! * 3^7.
// Distances fit in 4 bits and are stored two per byte: even keys use the low
// nibble, odd keys the high nibble, and 0xF means unvisited.
struct CornerDb {
  std::vector<uint8_t> data;

  CornerDb();
  uint8_t get(uint32_t key) const;
  void set(uint32_t key, uint8_t dist);
};

CornerDb buildCornerDb();

// Distance to the nearest solved corner state; 255 if not in the database.
uint32_t cornerHeuristic(const Cube &cube, const CornerDb &db);

bool saveCornerDb(const CornerDb &db, const std::string &path);
std::optional<CornerDb> loadCornerDb(const std::string &path);

#endif
