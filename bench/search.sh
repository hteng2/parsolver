#!/bin/sh
# Benchmark: time the first solution (-n 1) for every PLL setup in
# pll/setups.tsv, with and without the corner pattern database.
#
#   sh bench/search.sh [timeout-seconds]      # default 20
#
# The binary and the database (bench/db, ~25 s) are built when missing. Delete
# bench/db to rebuild it after changing the database format. Cases that do not
# finish within the timeout are recorded as TIMEOUT - without it the
# no-database column would never terminate, since it searches every bound from
# zero. No -l is passed: every run is bounded by -n 1 and the timeout.
#
# Raw results go to bench/search.tsv.

set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
BIN=$ROOT/build/parsolver
DB=$ROOT/bench/db
CASES=$ROOT/bench/pll/setups.tsv
OUT=$ROOT/bench/search.tsv
LIMIT=${1:-20}
TMPOUT=$(mktemp)
TMPTIME=$(mktemp)
trap 'rm -f "$TMPOUT" "$TMPTIME"' EXIT

if [ ! -x "$BIN" ]; then
  echo "building parsolver"
  make -C "$ROOT" > /dev/null || exit 1
fi

if [ ! -f "$DB" ]; then
  echo "building database $DB"
  "$BIN" build-db "$DB" || exit 1
fi

# Runs one case. Sets STATUS, ELAPSED, USERCPU and SOLUTION.
# $1 = db | nodb, $2 = moves, $3 = move group
solve_case() {
  if [ "$1" = nodb ]; then
    /usr/bin/time -f "%e %U %S" -o "$TMPTIME" timeout "$LIMIT" \
      "$BIN" solve -m "$2" -s "$3" -n 1 > "$TMPOUT" 2> /dev/null
  else
    /usr/bin/time -f "%e %U %S" -o "$TMPTIME" timeout "$LIMIT" \
      "$BIN" solve -p "$DB" -m "$2" -s "$3" -n 1 > "$TMPOUT" 2> /dev/null
  fi
  STATUS=$?
  ELAPSED=0
  USERCPU=0
  read -r ELAPSED USERCPU SYSCPU < "$TMPTIME" || true
  SOLUTION=$(sed -n 's/^solution: *//p' "$TMPOUT" | head -n 1)
  if [ "$STATUS" -eq 124 ]; then
    ELAPSED=TIMEOUT
    USERCPU=TIMEOUT
    SOLUTION=
  elif [ "$STATUS" -ne 0 ] || [ -z "$SOLUTION" ]; then
    ELAPSED=ERROR
    USERCPU=ERROR
    SOLUTION=
  fi
}

printf 'case\tmoves\tgroup\tdb_elapsed\tdb_user\tnodb_elapsed\tnodb_user\tdb_solution\tnodb_solution\n' > "$OUT"
echo "timeout: ${LIMIT}s per run"
printf '%-4s %8s %8s  %s\n' case 'db (s)' 'no db (s)' 'solution'

TAB=$(printf '\t')
while IFS="$TAB" read -r name moves group; do
  case $name in
    '' | '#'*) continue ;;
  esac

  solve_case db "$moves" "$group"
  db_elapsed=$ELAPSED
  db_user=$USERCPU
  db_solution=$SOLUTION

  solve_case nodb "$moves" "$group"
  nodb_elapsed=$ELAPSED
  nodb_user=$USERCPU
  nodb_solution=$SOLUTION

  printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
    "$name" "$moves" "$group" "$db_elapsed" "$db_user" \
    "$nodb_elapsed" "$nodb_user" "$db_solution" "$nodb_solution" >> "$OUT"

  printf '%-4s %8s %8s  %s\n' "$name" "$db_elapsed" "$nodb_elapsed" "$db_solution"
done < "$CASES"

awk -F'\t' '
function median(a, n,   i, j, t) {
  if (n == 0) return 0
  for (i = 2; i <= n; i++) {
    t = a[i]
    j = i - 1
    while (j >= 1 && a[j] > t) { a[j + 1] = a[j]; j-- }
    a[j + 1] = t
  }
  if (n % 2) return a[(n + 1) / 2]
  return (a[n / 2] + a[n / 2 + 1]) / 2
}
NR == 1 { next }
{
  n++
  if ($4 != "TIMEOUT") { dn++; dsum += $4; dt[dn] = $4 } else dto++
  if ($6 != "TIMEOUT") { nn++; nsum += $6; nt[nn] = $6 } else nto++
  if ($4 != "TIMEOUT" && $6 != "TIMEOUT") {
    p++
    pd += $4
    pn += $6
  }
  if ($4 != "TIMEOUT" && $6 != "TIMEOUT" && $8 != $9) mismatches++
}
END {
  printf "\ncases:                    %d\n", n
  printf "with database:            %d solved, %d timed out; total %.2f s, mean %.3f s, median %.3f s\n", dn, dto, dsum, dn ? dsum / dn : 0, median(dt, dn)
  printf "without database:         %d solved, %d timed out; total %.2f s, mean %.3f s, median %.3f s\n", nn, nto, nsum, nn ? nsum / nn : 0, median(nt, nn)
  if (p) printf "solved both ways:         %d cases, %.2f s with database vs %.2f s without\n", p, pd, pn
  if (mismatches) printf "WARNING: %d cases gave different solutions with and without the database\n", mismatches
}
' "$OUT"
