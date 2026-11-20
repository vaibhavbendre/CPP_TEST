// Dependency-free tests: no framework, just a CHECK macro and a main() that returns non-zero on failure.
// Run with `ctest` or by executing cinema_tests directly.
#include <algorithm>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <set>
#include <sstream>
#include <thread>

#include "cinema/booking_service.hpp"
#include "cinema/cli.hpp"
#include "cinema/seat.hpp"

using namespace cinema;
using Seats = std::vector<std::string>;

static int failures = 0;
#define CHECK(cond)                                                                  \
    do {                                                                             \
        if (!(cond)) {                                                               \
            ++failures;                                                              \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n";  \
        }                                                                            \
    } while (0)

// Movies 1,2,3 ; theaters 1,2,3 ; showings (1,1) (1,2) (2,2). Movie 2 is NOT in theater 1; movie 3 is nowhere.
static BookingService makeService() {
    return BookingService({{1, "A"}, {2, "B"}, {3, "C"}}, {{1, "T1"}, {2, "T2"}, {3, "T3"}},
                          {{1, 1}, {1, 2}, {2, 2}});
}

static size_t freeCount(BookingService& s, MovieId m, TheaterId t) { return s.availableSeats(m, t)->size(); }

static void testSeats() {
    CHECK(kSeatCount == 20);
    CHECK(seatLabel(0) == "a1" && seatLabel(4) == "a5" && seatLabel(5) == "b1" && seatLabel(19) == "d5");
    std::set<std::string> labels;
    for (size_t i = 0; i < kSeatCount; ++i) {
        labels.insert(seatLabel(i));
        CHECK(parseSeat(seatLabel(i)) == i);  // round trip
    }
    CHECK(labels.size() == 20);
    CHECK(parseSeat("A1") == parseSeat("a1"));
    for (const char* bad : {"", "a", "1", "1a", "a0", "a6", "a01", "a10", "e1", "aa", " a1", "a1 ", "a-1", "a1x"}) {
        CHECK(!parseSeat(bad));
    }
    bool threw = false;
    try { seatLabel(kSeatCount); } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
}

static void testListing() {
    auto s = makeService();
    CHECK(s.listMovies().size() == 3);
    CHECK(s.listTheaters(1).size() == 2 && s.listTheaters(1)[0].id == 1 && s.listTheaters(1)[1].id == 2);
    CHECK(s.listTheaters(2).size() == 1);
    CHECK(s.listTheaters(3).empty() && s.listTheaters(99).empty());
    CHECK(freeCount(s, 1, 1) == 20 && s.availableSeats(1, 1)->front() == "a1" && s.availableSeats(1, 1)->back() == "d5");
    CHECK(!s.availableSeats(2, 1) && !s.availableSeats(99, 1) && !s.availableSeats(1, 99));
}

static void testBooking() {
    auto s = makeService();
    auto r = s.book(1, 1, {"d5", "A1", "a2"});
    CHECK(r.ok() && r.bookingId != 0 && r.seats == (Seats{"a1", "a2", "d5"}));  // canonical, seat order
    CHECK(freeCount(s, 1, 1) == 17);
    CHECK(freeCount(s, 1, 2) == 20 && freeCount(s, 2, 2) == 20);  // other showings untouched

    auto again = s.book(1, 1, {"a1"});  // double booking
    CHECK(again.status == BookingStatus::SeatsUnavailable && again.bookingId == 0 && again.seats == Seats{"a1"});

    auto partial = s.book(1, 1, {"b1", "a1", "b2"});  // all-or-nothing
    CHECK(partial.status == BookingStatus::SeatsUnavailable && partial.seats == Seats{"a1"});
    CHECK(freeCount(s, 1, 1) == 17);

    auto next = s.book(1, 1, {"c1"});  // ids: unique, consecutive, not burned by failures
    CHECK(next.ok() && next.bookingId == r.bookingId + 1);

    Seats all;  // sell out; "sold out" (empty) differs from "no such showing" (nullopt)
    for (size_t i = 0; i < kSeatCount; ++i) all.push_back(seatLabel(i));
    CHECK(s.book(1, 2, all).ok() && s.availableSeats(1, 2)->empty());
}

static void testFailures() {
    auto s = makeService();
    CHECK(s.book(1, 1, {}).status == BookingStatus::EmptyRequest);
    CHECK(s.book(99, 1, {"a1"}).status == BookingStatus::UnknownMovie);
    CHECK(s.book(1, 99, {"a1"}).status == BookingStatus::UnknownTheater);
    CHECK(s.book(2, 1, {"a1"}).status == BookingStatus::NotShowing);
    auto bad = s.book(1, 1, {"a1", "z9", "a0"});
    CHECK(bad.status == BookingStatus::InvalidSeat && bad.seats == (Seats{"z9", "a0"}));
    auto dup = s.book(1, 1, {"a1", "A1", "b1", "b1", "b1"});
    CHECK(dup.status == BookingStatus::DuplicateSeat && dup.seats == (Seats{"a1", "b1"}));
    CHECK(freeCount(s, 1, 1) == 20);  // no failure may block anything
    // validation order: empty > movie > theater > showing > label > duplicate
    CHECK(s.book(99, 99, {}).status == BookingStatus::EmptyRequest);
    CHECK(s.book(99, 99, {"zz"}).status == BookingStatus::UnknownMovie);
    CHECK(s.book(2, 1, {"zz"}).status == BookingStatus::NotShowing);
    CHECK(s.book(1, 1, {"a1", "a1", "zz"}).status == BookingStatus::InvalidSeat);
    for (auto st : {BookingStatus::Ok, BookingStatus::EmptyRequest, BookingStatus::UnknownMovie,
                    BookingStatus::UnknownTheater, BookingStatus::NotShowing, BookingStatus::InvalidSeat,
                    BookingStatus::DuplicateSeat, BookingStatus::SeatsUnavailable}) {
        CHECK(std::string(toString(st)).size() > 0);
    }
}

static void testConstruction() {
    auto throws = [](std::vector<Movie> m, std::vector<Theater> t, std::vector<Showing> sh) {
        try { BookingService s(m, t, sh); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    CHECK(throws({{1, "A"}, {1, "B"}}, {{1, "T"}}, {}));
    CHECK(throws({{1, "A"}}, {{1, "T"}, {1, "U"}}, {}));
    CHECK(throws({{1, "A"}}, {{1, "T"}}, {{2, 1}}));
    CHECK(throws({{1, "A"}}, {{1, "T"}}, {{1, 2}}));
    CHECK(throws({{1, "A"}}, {{1, "T"}}, {{1, 1}, {1, 1}}));
    CHECK(!throws({}, {}, {}));
}

// Releases all threads at once (start gate) to maximise real interleaving.
template <typename Fn>
static void race(int threads, Fn body) {
    std::atomic<bool> go{false};
    std::vector<std::thread> pool;
    for (int i = 0; i < threads; ++i) {
        pool.emplace_back([&, i] { while (!go) std::this_thread::yield(); body(i); });
    }
    go = true;
    for (auto& t : pool) t.join();
}

static void testConcurrency() {
    // 64 threads, ONE seat -> exactly one winner. Repeated to give a race many chances.
    for (int round = 0; round < 50; ++round) {
        auto s = makeService();
        std::atomic<int> wins{0};
        race(64, [&](int) { if (s.book(1, 1, {"c3"}).ok()) ++wins; });
        CHECK(wins == 1);
    }
    // 20 threads, 20 distinct seats -> all succeed, ids unique.
    {
        auto s = makeService();
        std::vector<BookingId> ids(kSeatCount, 0);
        race(static_cast<int>(kSeatCount), [&](int i) { ids[i] = s.book(1, 1, {seatLabel(i)}).bookingId; });
        CHECK(std::set<BookingId>(ids.begin(), ids.end()).size() == kSeatCount && ids[0] != 0);
        CHECK(s.availableSeats(1, 1)->empty());
    }
    // Overlapping multi-seat requests: no seat granted twice, none lost.
    for (int round = 0; round < 20; ++round) {
        auto s = makeService();
        std::vector<Seats> granted(16);
        race(16, [&](int t) {
            unsigned x = 12345u + 7919u * t + 104729u * round;  // tiny deterministic PRNG
            for (int k = 0; k < 30; ++k) {
                Seats req;
                for (int n = 0, want = 1 + (x >>= 3) % 4; n < want; ++n) {
                    x = x * 1103515245u + 12345u;
                    const std::string seat = seatLabel((x >> 16) % kSeatCount);
                    if (std::find(req.begin(), req.end(), seat) == req.end()) req.push_back(seat);
                }
                auto r = s.book(1, 1, req);
                if (r.ok()) granted[t].insert(granted[t].end(), r.seats.begin(), r.seats.end());
            }
        });
        std::multiset<std::string> all;
        for (auto& g : granted) all.insert(g.begin(), g.end());
        CHECK(std::set<std::string>(all.begin(), all.end()).size() == all.size());  // no double grant
        CHECK(all.size() + freeCount(s, 1, 1) == kSeatCount);                       // nothing lost
    }
    // Readers and writers together: free count only ever shrinks.
    {
        auto s = makeService();
        std::atomic<bool> monotonic{true};
        std::atomic<int> wins{0};
        race(12, [&](int i) {
            if (i < 4) {
                size_t last = kSeatCount;
                for (int k = 0; k < 2000; ++k) {
                    size_t now = freeCount(s, 1, 1);
                    if (now > last) monotonic = false;
                    last = now;
                    s.listMovies();
                    s.listTheaters(1);
                }
            } else {
                for (size_t n = 0; n < kSeatCount; ++n) if (s.book(1, 1, {seatLabel(n)}).ok()) ++wins;
            }
        });
        CHECK(monotonic && wins == 20);
    }
}

static std::string runCliScript(BookingService& s, const std::string& script) {
    std::istringstream in(script);
    std::ostringstream out;
    runCli(s, in, out);
    return out.str();
}
static bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

static void testCli() {
    auto s = makeService();
    CHECK(has(runCliScript(s, "q\n"), "1) A") && has(runCliScript(s, ""), "Goodbye"));  // EOF quits
    auto out = runCliScript(s, "1\n1\na1, A2 b1\nzz\na1\nb\nb\n2\nq\n");
    CHECK(has(out, "1) T1") && has(out, "2) T2"));                       // theaters for movie 1
    CHECK(has(out, "Booking confirmed #1: a1, a2, b1"));                // comma + mixed case input
    CHECK(has(out, "Invalid seat label: zz"));
    CHECK(has(out, "Some of the seats are already booked: a1"));
    CHECK(has(out, "Theaters showing B"));                              // 'b' went back twice, then picked movie 2
    CHECK(freeCount(s, 1, 1) == 17);
    out = runCliScript(s, "abc\n99\n-1\n1\nnope\nq\n");
    CHECK(has(out, "listed movie ids") && has(out, "listed theater ids"));
}

// ---- Exception safety (pre-ship review finding F11) ------------------------------------------
// Replace the global allocator so a test can make the Nth allocation throw std::bad_alloc.
// The headers are included here, next to the only code that needs them.
#include <cstdlib>
#include <new>

static std::atomic<int> g_failCountdown{0};  // 0 = disarmed; N > 0 = the Nth allocation from now throws

void* operator new(std::size_t n) {
    if (g_failCountdown > 0 && --g_failCountdown == 0) throw std::bad_alloc();
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

// An exception inside book() must never leave seats blocked behind it: either the caller gets a
// result, or nothing was booked and a retry works. Fail the 1st, 2nd, ... allocation in turn.
static void testExceptionSafety() {
    const Seats request{"a1", "b2", "c3"};
    for (int nth = 1; nth <= 12; ++nth) {
        auto s = makeService();
        bool threw = false;
        BookingResult r;
        g_failCountdown = nth;
        try {
            r = s.book(1, 1, request);
        } catch (const std::bad_alloc&) {
            threw = true;
        }
        g_failCountdown = 0;  // disarm before any check allocates
        if (threw) {
            CHECK(freeCount(s, 1, 1) == 20);      // nothing may stay blocked behind an exception
            CHECK(s.book(1, 1, request).ok());    // so a retry succeeds
        } else {
            CHECK(r.ok() && freeCount(s, 1, 1) == 17);
        }
    }
}


int main() {
    testSeats();
    testListing();
    testBooking();
    testFailures();
    testConstruction();
    testConcurrency();
    testCli();
    testExceptionSafety();
    if (failures) {
        std::cerr << failures << " check(s) FAILED\n";
        return 1;
    }
    std::cout << "All tests passed\n";
    return 0;
}
