# Movie Ticket Booking Backend (C++17)

An in-memory, thread-safe C++ library for booking movie tickets, plus a small interactive
CLI that embeds it. Booking a ticket means atomically blocking particular seats; concurrent
requests can never over-book a seat.

**Not a programmer? Read [`HOW_TO_RUN.md`](HOW_TO_RUN.md).** The design and the code review are in
[`design.md`](design.md).

| File | What it is |
|---|---|
| `include/cinema/` | Public, documented API headers (the contract for consumers) |
| `src/` | Implementation (`seat`, `booking_service`, `cli`) |
| `app/main.cpp` | CLI entry point, contains the demo catalogue |
| `tests/tests.cpp` | All tests, one file, no test framework |

## API

| Brief requirement | Call |
|---|---|
| View all playing movies | `listMovies()` |
| Select a movie, see theaters showing it | `listTheaters(movieId)` |
| Select a theater, see available seats | `availableSeats(movieId, theaterId)` |
| Book one or more seats (no over-booking) | `book(movieId, theaterId, {"a1","a2"})` |

Seats are `a1..a5, b1..b5, c1..c5, d1..d5` (20 per theater, case-insensitive on input).
Booking is all-or-nothing. Every member function is thread-safe. Failures are returned as a
`BookingStatus`, not thrown.

```cpp
#include "cinema/booking_service.hpp"

cinema::BookingService svc({{1, "The Matrix"}},                  // movies
                           {{1, "Downtown"}, {2, "Riverside"}},  // theaters
                           {{1, 1}, {1, 2}});                    // (movie, theater) showings
auto theaters = svc.listTheaters(1);
auto freeSeats = svc.availableSeats(1, 1);          // optional<vector<string>>
auto r = svc.book(1, 1, {"a1", "a2"});              // r.ok(), r.bookingId, r.seats, r.status
```

## Build, test, run

Needs a C++17 compiler and CMake >= 3.16. **No other dependency**; the tests use only the
standard library.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
./build/cinema_cli                 # Windows (Ninja): build\cinema_cli.exe
                                   # Windows (Visual Studio generator): build\Release\cinema_cli.exe
```

Windows shortcut (Visual Studio 2022 installed): double-click `build_windows.bat`, then `run_cinema.bat`.

Linux shortcut: run `build_linux.sh` script in the root directory.

On Windows the library links the C++ runtime **statically by default**. If your own program uses the dynamic runtime (`/MD`,
the Visual Studio default), configure with `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL`; otherwise linking fails with `LNK2038`.
`conan create` does this for you.
Piped input works too (see `HOW_TO_RUN.md`).

## Bonuses

* **Conan 2:** `conan create . --build=missing` builds, tests and packages the library. I have attempted to solve this bonus section but I am not comfortable at the moment using it so unsure of the accuracy.
  (`find_package(cinema)` -> `cinema::cinema`). See `conanfile.py`.
* **Doxygen:** `doxygen Doxyfile` (or `cmake --build build --target docs`) -> `docs/html/index.html`.


`build/` and `docs/html/` are generated output.


## Reflection Questions
#### Question 1 : What aspects of this exercise did you find the most interesting?

**Answer -** The most important interesting part was choosing a representation that made the hard property fall out of the data structure. Seats as bit mask turn "are all of these seats free?" into `wanted and booked` and "block them" into `booked | = wanted`. One mutex then makes atomic multi-seat booking trivial and removes any rollback path. Meaning the atomicity, isolation and Consistency because handling multiple requests simulatneously.

The second interesting point was exception safety. A naive first version of the code used to issue `bad_alloc` could leave seats blocked with no booking returned. Moving every allocation ahead of the commiy fixed it without changing the existing behavior. This part was interesting as it took me a while to figure this out.

* **Trade Off -** 
I am using a bit mask and a mask doesn't scale beyond 64 seats and `std::bitset` or per-seat state would be the next step. The usage of bitmask is just a design decision for a provided constraint and a tidy design.

To scale this solution say about 200 seats or something similar we need to find a slightly different approach. The most important aspect of this code is exception safety and cebtral to the design.


#### Question 2 : What did you find most cumbersome?
**Answer -** Three things
1. **The underspecification of the design :** The seat layout, what 'select' means and whether there are showtimes all had to be assumed. Every assumption changes design decisions and puts documentation effort.

2. **Proving Concurrency :** Tests can show a race exists but there is absolutely no way to prove it's absence. I mitigated that with start-gate threads, repeated rounds, and mutation testing the lock to confirm the tests would fail without it. The tests did not include a ThreadSanitizer run, which would be the correct additional evidence.

3. **Cross Platform Testing** : Cross platform testing was bit cumbersome. 

4. All ambiguities in assumptions were resolved using design decision hedge.
