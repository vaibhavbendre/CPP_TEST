#!/usr/bin/env bash
# Starts the movie ticket booking program on Linux. Linux counterpart of run_cinema.bat.
# Build it first with ./build_linux.sh. Piped input works too, for example:
#   printf '1\n1\na1 a2\nq\n' | ./run_cinema.sh
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"

EXE=""
for candidate in "${BUILD_DIR:-$HERE/build-linux}/cinema_cli" "$HERE/cinema_cli"; do
    if [ -x "$candidate" ] && [ -f "$candidate" ]; then
        EXE="$candidate"
        break
    fi
done

if [ -z "$EXE" ]; then
    echo "Could not find the cinema_cli program. Build it first with: ./build_linux.sh" >&2
    echo "(See HOW_TO_RUN.md.)" >&2
    exit 1
fi

exec "$EXE" "$@"
