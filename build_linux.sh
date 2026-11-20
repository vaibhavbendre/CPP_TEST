#!/usr/bin/env bash
# Builds cinema_cli and runs the tests on Linux. Written for Ubuntu (20.04 and newer) and
# openSUSE Leap 15.3, and it uses nothing distribution specific, so other Linux systems work too.
# Linux counterpart of build_windows.bat.
#
# Usage:   ./build_linux.sh [--clean]      (or: bash build_linux.sh)
#   --clean        delete the build folder first
# Optional environment variables:
#   CXX=g++-10     use this C++ compiler instead of searching for one
#   BUILD_TYPE=Debug   Release (the default) or Debug
#   BUILD_DIR=out  build folder (default: build-linux, kept apart from the Windows "build" folder
#                  because a CMake folder remembers the system and compiler it was created with)
#
# Why not "ctest --test-dir": that option needs CMake 3.20 and openSUSE Leap 15.3 has 3.17,
# so the script runs ctest from inside the build folder instead.
set -euo pipefail

cd "$(dirname "$0")"

BUILD_DIR="${BUILD_DIR:-build-linux}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
OS_RELEASE="${OS_RELEASE_FILE:-/etc/os-release}"   # overridable only so the hints can be tested

# ---- helpers -------------------------------------------------------------------------------

# Print the install command for the packages this script needs, matched to the distribution.
install_hint() {
    local id="" like=""
    if [ -r "$OS_RELEASE" ]; then
        id=$(. "$OS_RELEASE"; echo "${ID:-}")
        like=$(. "$OS_RELEASE"; echo "${ID_LIKE:-}")
    fi
    case "$id $like" in
        *ubuntu*|*debian*)
            echo "  sudo apt-get update && sudo apt-get install -y build-essential cmake" ;;
        *suse*|*sles*)
            echo "  sudo zypper install -y gcc-c++ cmake make"
            echo "  Leap 15.3 ships g++ 7.5 by default. A newer compiler is recommended, and this script uses it automatically:"
            echo "  sudo zypper install -y gcc10-c++     (the script then prefers g++-10 over g++)" ;;
        *)
            echo "  Install a C++17 compiler (g++ 8 or newer, or clang++ 7 or newer), CMake 3.16 or newer and make." ;;
    esac
}

die() {
    echo "ERROR: $*" >&2
    exit 1
}

# True if "$1" is a C++ compiler that accepts -std=c++17 and has <optional> (the code needs both).
supports_cxx17() {
    printf '#include <optional>\nint main() { std::optional<int> o(1); return *o == 1 ? 0 : 1; }\n' |
        "$1" -std=c++17 -x c++ - -o "$PROBE_DIR/probe" >/dev/null 2>&1
}

# Print the first usable compiler: $CXX if set, else the newest g++ / clang++ that supports C++17.
pick_compiler() {
    if [ -n "${CXX:-}" ]; then
        command -v "$CXX" >/dev/null 2>&1 || die "CXX='$CXX' was not found."
        supports_cxx17 "$CXX" || die "CXX='$CXX' cannot compile C++17 (<optional> is missing or -std=c++17 is rejected)."
        echo "$CXX"
        return 0
    fi
    local c
    for c in g++-14 g++-13 g++-12 g++-11 g++-10 g++-9 g++-8 g++ c++ clang++; do
        if command -v "$c" >/dev/null 2>&1 && supports_cxx17 "$c"; then
            echo "$c"
            return 0
        fi
    done
    return 1
}

# ---- checks --------------------------------------------------------------------------------

if [ "$(uname -s)" != "Linux" ]; then
    echo "Note: this script is written for Linux (this system reports '$(uname -s)'). Continuing anyway." >&2
fi

PROBE_DIR=$(mktemp -d)
trap 'rm -rf "$PROBE_DIR"' EXIT

if ! command -v cmake >/dev/null 2>&1; then
    echo "CMake was not found. To install the build tools:" >&2
    install_hint >&2
    exit 1
fi

# "cmake version 3.17.0" -> major 3, minor 17. The project needs 3.16 or newer.
cmake_version=$(cmake --version | sed -n '1s/[^0-9]*\([0-9][0-9]*\)\.\([0-9][0-9]*\).*/\1 \2/p')
cmake_major=${cmake_version% *}
cmake_minor=${cmake_version#* }
if [ -z "$cmake_version" ] || [ "$cmake_major" -lt 3 ] || { [ "$cmake_major" -eq 3 ] && [ "$cmake_minor" -lt 16 ]; }; then
    die "CMake 3.16 or newer is required (found: $(cmake --version | head -n 1))."
fi

if ! COMPILER=$(pick_compiler); then
    # pick_compiler already explained the problem when the user chose a compiler with CXX.
    [ -n "${CXX:-}" ] && exit 1
    echo "No C++17 compiler was found. To install the build tools:" >&2
    install_hint >&2
    exit 1
fi
echo "Using compiler: $COMPILER ($("$COMPILER" --version | head -n 1))"
echo "Using CMake:    $(cmake --version | head -n 1)"

# Ninja is faster if present; otherwise CMake falls back to its default (Unix Makefiles).
GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
    GENERATOR_ARGS=(-G Ninja)
fi

JOBS=$(nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

# ---- build and test -------------------------------------------------------------------------

if [ "${1:-}" = "--clean" ]; then
    echo "Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

cmake -S . -B "$BUILD_DIR" "${GENERATOR_ARGS[@]+"${GENERATOR_ARGS[@]}"}" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_CXX_COMPILER="$COMPILER"
cmake --build "$BUILD_DIR" -j "$JOBS"
(cd "$BUILD_DIR" && ctest --output-on-failure)

# Files copied from Windows (or unpacked from a zip) lose the "executable" bit; restore it so
# ./run_cinema.sh works. Harmless if it fails (for example on a read-only checkout).
chmod +x build_linux.sh run_cinema.sh 2>/dev/null || true

echo
echo "Build OK. Start the program with ./run_cinema.sh"
