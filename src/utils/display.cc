#include "utils/display.h"
#include "lib/cube.h"

#include <cstdint>
#include <ostream>

static uint8_t nibbleGet(const uint8_t *array, int i) {
  return (array[i >> 1] >> ((i & 1) * 4)) & 0xF;
}

static char charFromColor(Color c) {
  switch (c) {
  case Color::White:
    return 'w';
  case Color::Yellow:
    return 'y';
  case Color::Green:
    return 'g';
  case Color::Blue:
    return 'b';
  case Color::Red:
    return 'r';
  case Color::Orange:
    return 'o';
  case Color::Gray:
    return 'd';
  case Color::None:
    return '?';
  }
}

static const char *ansiFromColor(Color c) {
  switch (c) {
  case Color::White:
    return "\x1b[97m";
  case Color::Yellow:
    return "\x1b[93m";
  case Color::Green:
    return "\x1b[92m";
  case Color::Blue:
    return "\x1b[94m";
  case Color::Red:
    return "\x1b[91m";
  case Color::Orange:
    return "\x1b[38;5;208m";
  case Color::Gray:
    return "\x1b[90m";
  case Color::None:
    return "\x1b[0m";
  }
}

// Facelets of each corner position, in the same slot order used by the corner
// orientation convention (slot 0 is the U/D facelet).
static const int kCornerFacets[8][3] = {
    {8, 20, 36},  // UFR
    {2, 38, 27},  // URB
    {0, 29, 45},  // UBL
    {6, 47, 18},  // ULF
    {11, 42, 26}, // DFR
    {17, 33, 44}, // DRB
    {15, 51, 35}, // DBL
    {9, 24, 53},  // DLF
};

// Sticker colours of each corner piece, in its own slot order.
static const Color kCornerColors[8][3] = {
    {Color::White, Color::Green, Color::Red},     // UFR
    {Color::White, Color::Red, Color::Blue},      // URB
    {Color::White, Color::Blue, Color::Orange},   // UBL
    {Color::White, Color::Orange, Color::Green},  // ULF
    {Color::Yellow, Color::Red, Color::Green},    // DFR
    {Color::Yellow, Color::Blue, Color::Red},     // DRB
    {Color::Yellow, Color::Orange, Color::Blue},  // DBL
    {Color::Yellow, Color::Green, Color::Orange}, // DLF
};

// Facelets of each edge position, primary face first.
static const int kEdgeFacets[12][2] = {
    {7, 19},  // UF
    {5, 37},  // UR
    {1, 28},  // UB
    {3, 46},  // UL
    {23, 39}, // FR
    {21, 50}, // FL
    {30, 41}, // BR
    {32, 48}, // BL
    {10, 25}, // DF
    {14, 43}, // DR
    {16, 34}, // DB
    {12, 52}, // DL
};

// Sticker colours of each edge piece, primary colour first.
static const Color kEdgeColors[12][2] = {
    {Color::White, Color::Green},   // UF
    {Color::White, Color::Red},     // UR
    {Color::White, Color::Blue},    // UB
    {Color::White, Color::Orange},  // UL
    {Color::Green, Color::Red},     // FR
    {Color::Green, Color::Orange},  // FL
    {Color::Blue, Color::Red},      // BR
    {Color::Blue, Color::Orange},   // BL
    {Color::Yellow, Color::Green},  // DF
    {Color::Yellow, Color::Red},    // DR
    {Color::Yellow, Color::Blue},   // DB
    {Color::Yellow, Color::Orange}, // DL
};

// Center facelets and the colours of the six center pieces (U, D, F, B, R, L).
static const int kCenterFacets[6] = {4, 13, 22, 31, 40, 49};
static const Color kCenterColors[6] = {
    Color::White, Color::Yellow, Color::Green,
    Color::Blue,  Color::Red,    Color::Orange,
};

/**
 * Rebuilds the 54 facelets from the cubie state.
 *
 *          00 01 02
 *          03 04 05
 *          06 07 08
 * 45 46 47 18 19 20 36 37 38 27 28 29
 * 48 49 50 21 22 23 39 40 41 30 31 32
 * 51 52 53 24 25 26 42 43 44 33 34 35
 *          09 10 11
 *          12 13 14
 *          15 16 17
 */
static void buildFacets(const Cube &cube, Color (&facets)[54]) {
  for (int i = 0; i < 54; i++) {
    facets[i] = Color::None;
  }

  for (int pos = 0; pos < 8; pos++) {
    int piece = nibbleGet(cube.cornerPerm, pos);
    int ori = nibbleGet(cube.cornerOri, pos);
    int slot = (3 - ori) % 3; // slot holding the piece's U/D sticker
    for (int k = 0; k < 3; k++) {
      facets[kCornerFacets[pos][(slot + k) % 3]] = kCornerColors[piece][k];
    }
  }

  for (int pos = 0; pos < 12; pos++) {
    int piece = nibbleGet(cube.edgePerm, pos);
    int flipped = nibbleGet(cube.edgeOri, pos);
    facets[kEdgeFacets[pos][0]] = kEdgeColors[piece][flipped ? 1 : 0];
    facets[kEdgeFacets[pos][1]] = kEdgeColors[piece][flipped ? 0 : 1];
  }

  for (int pos = 0; pos < 6; pos++) {
    int piece = nibbleGet(cube.centerPerm, pos);
    facets[kCenterFacets[pos]] = kCenterColors[piece];
  }
}

static void printFaceRow(std::ostream &os, const Color *facets, int base,
                         int row) {
  for (int col = 0; col < 3; col++) {
    Color c = facets[base + row * 3 + col];
    os << ansiFromColor(c) << charFromColor(c) << "\x1b[0m ";
  }
}

void printCube(std::ostream &os, const Cube &cube) {
  Color facets[54];
  buildFacets(cube, facets);

  for (int row = 0; row < 3; row++) {
    os << "      ";
    printFaceRow(os, facets, 0, row);
    os << '\n';
  }
  for (int row = 0; row < 3; row++) {
    printFaceRow(os, facets, 45, row);
    printFaceRow(os, facets, 18, row);
    printFaceRow(os, facets, 36, row);
    printFaceRow(os, facets, 27, row);
    os << '\n';
  }
  for (int row = 0; row < 3; row++) {
    os << "      ";
    printFaceRow(os, facets, 9, row);
    os << '\n';
  }
}
