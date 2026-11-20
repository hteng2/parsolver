#include "db/patterns.h"
#include "lib/cube.h"
#include "lib/turn.h"
#include "solve/search.h"
#include "utils/display.h"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>

static void usage() {
  std::cerr << "usage:\n"
            << "  parsolver solve -m <moves> -s <moveset> [-l <length>] "
               "[-n <solutions>] [-p <db>]\n"
            << "  parsolver build-db <path>\n";
}

static int runSolve(int argc, char **argv) {
  std::string moves;
  std::string moveset;
  std::string length;
  std::string solutionCount;
  std::string dbPath;
  bool haveMoves = false;
  bool haveMoveset = false;
  bool haveLength = false;
  bool haveSolutions = false;
  bool haveDbPath = false;

  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "-m") {
      if (i + 1 >= argc) {
        std::cerr << "error: -m requires a move string\n";
        return 1;
      }
      moves = argv[++i];
      haveMoves = true;
    } else if (arg == "-s") {
      if (i + 1 >= argc) {
        std::cerr << "error: -s requires a moveset string\n";
        return 1;
      }
      moveset = argv[++i];
      haveMoveset = true;
    } else if (arg == "-l") {
      if (i + 1 >= argc) {
        std::cerr << "error: -l requires a search length\n";
        return 1;
      }
      length = argv[++i];
      haveLength = true;
    } else if (arg == "-n") {
      if (i + 1 >= argc) {
        std::cerr << "error: -n requires a solution count\n";
        return 1;
      }
      solutionCount = argv[++i];
      haveSolutions = true;
    } else if (arg == "-p") {
      if (i + 1 >= argc) {
        std::cerr << "error: -p requires a database file path\n";
        return 1;
      }
      dbPath = argv[++i];
      haveDbPath = true;
    } else {
      std::cerr << "error: unknown argument '" << arg << "'\n";
      return 1;
    }
  }

  if (!haveMoves || !haveMoveset) {
    std::cerr << "error: -m and -s are required\n";
    return 1;
  }

  int maxLength = 32;
  if (haveLength) {
    const char *begin = length.data();
    const char *end = begin + length.size();
    std::from_chars_result parsed = std::from_chars(begin, end, maxLength);
    if (parsed.ec != std::errc() || parsed.ptr != end || maxLength < 0 ||
        maxLength > 32) {
      std::cerr << "error: -l must be an integer between 0 and 32\n";
      return 1;
    }
  }

  uint64_t maxSolutions = 0;
  if (haveSolutions) {
    const char *begin = solutionCount.data();
    const char *end = begin + solutionCount.size();
    std::from_chars_result parsed = std::from_chars(begin, end, maxSolutions);
    if (parsed.ec != std::errc() || parsed.ptr != end) {
      std::cerr << "error: -n must be a non-negative integer\n";
      return 1;
    }
  }

  Moveset moveSet = movesetFromString(moveset);
  std::optional<CornerDb> db;
  if (haveDbPath) {
    db = loadCornerDb(dbPath);
    if (!db) {
      std::cerr << "error: cannot read corner database '" << dbPath << "'\n";
      return 1;
    }
  }

  Cube cube = cubeDefault();

  std::istringstream stream(moves);
  std::string token;
  while (stream >> token) {
    std::optional<Turn> turn = turnFromString(token);
    if (!turn) {
      std::cerr << "error: invalid move '" << token << "'\n";
      return 1;
    }
    doTurn(cube, *turn);
  }

  printCube(std::cout, cube);

  solve(cube, static_cast<uint8_t>(maxLength), moveSet, db ? &*db : nullptr,
        maxSolutions);

  return 0;
}

static int runBuildDb(int argc, char **argv) {
  if (argc != 3) {
    usage();
    return 1;
  }

  CornerDb db = buildCornerDb();
  if (!saveCornerDb(db, argv[2])) {
    std::cerr << "error: cannot write corner database '" << argv[2] << "'\n";
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
    return 1;
  }

  std::string command = argv[1];
  if (command == "solve") {
    return runSolve(argc, argv);
  }
  if (command == "build-db") {
    return runBuildDb(argc, argv);
  }

  usage();
  return 1;
}
