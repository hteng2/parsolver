#ifndef cube_h
#define cube_h

#include <cstdint>

enum class Color : uint8_t {
  White,
  Yellow,
  Green,
  Blue,
  Red,
  Orange,
  Gray,
  None,
};

Color colorFromChar(char c);

/**
 * Cubie representation. Every piece id and orientation is stored in one
 * nibble; in each array, even positions occupy the low nibble of a byte and
 * odd positions the high nibble (byte = index / 2, shift = (index % 2) * 4).
 *
 * Permutations are position-indexed: perm[i] is the id of the piece currently
 * at position i, where positions use the solved-cube piece ids below.
 *
 * Corner pieces (upright sticker first, then the other two in cyclic order):
 *   0 UFR  White  Green Red
 *   1 URB  White  Red   Blue
 *   2 UBL  White  Blue  Orange
 *   3 ULF  White  Orange Green
 *   4 DFR  Yellow Green Red
 *   5 DRB  Yellow Red   Blue
 *   6 DBL  Yellow Blue  Orange
 *   7 DLF  Yellow Orange Green
 *
 * Edge pieces:
 *    0 UF  White  Green
 *    1 UR  White  Red
 *    2 UB  White  Blue
 *    3 UL  White  Orange
 *    4 FR  Green  Red
 *    5 FL  Green  Orange
 *    6 BR  Blue   Red
 *    7 BL  Blue   Orange
 *    8 DF  Yellow Green
 *    9 DR  Yellow Red
 *   10 DB  Yellow Blue
 *   11 DL  Yellow Orange
 *
 * Center pieces:
 *   0 U White
 *   1 D Yellow
 *   2 F Green
 *   3 B Blue
 *   4 R Red
 *   5 L Orange
 *
 * Orientations:
 *   cornerOri[i] 0 = the piece's U/D sticker is on the U/D face,
 *                1 / 2 = twisted clockwise / counterclockwise.
 *   edgeOri[i]   0 = oriented, 1 = flipped. A U/D edge is oriented when its
 *                U/D sticker is on a U/D face; a middle-layer edge is
 *                oriented when its F/B sticker is on an F/B face.
 *   Centers have no orientation; centerPerm tracks whole-cube rotations.
 *
 * Nibble usage: cornerPerm 8 (4 bytes), cornerOri 8 (4 bytes), edgePerm 12
 * (6 bytes), edgeOri 12 (6 bytes), centerPerm 6 (3 bytes).
 */
struct Cube {
  uint8_t cornerPerm[4];
  uint8_t cornerOri[4];
  uint8_t edgePerm[6];
  uint8_t edgeOri[6];
  uint8_t centerPerm[3];
};

Cube cubeDefault();

#endif
