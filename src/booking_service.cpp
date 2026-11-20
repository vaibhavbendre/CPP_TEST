// BookingService implementation. See design.md sections 7 and 8.
#include "cinema/booking_service.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>

#include "cinema/seat.hpp"

namespace cinema {

const char* toString(BookingStatus status) {
    switch (status) {
        case BookingStatus::Ok:               return "Booking confirmed";
        case BookingStatus::EmptyRequest:     return "No seats were requested";
        case BookingStatus::UnknownMovie:     return "Unknown movie";
        case BookingStatus::UnknownTheater:   return "Unknown theater";
        case BookingStatus::NotShowing:       return "This movie is not showing in that theater";
        case BookingStatus::InvalidSeat:      return "Invalid seat label";
        case BookingStatus::DuplicateSeat:    return "The same seat was requested more than once";
        case BookingStatus::SeatsUnavailable: return "Some of the seats are already booked";
    }
    return "Unknown status";
}

namespace {
using Mask = std::uint32_t;  // bit i set <=> seat index i is booked / requested

constexpr Mask bit(std::size_t i) { return Mask{1} << i; }
constexpr Mask kAllSeats = static_cast<Mask>((std::uint64_t{1} << kSeatCount) - 1);

/// Labels of the seats whose bit is set in `mask`, in seat order.
std::vector<std::string> labelsOf(Mask mask) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < kSeatCount; ++i) {
        if (mask & bit(i)) out.push_back(seatLabel(i));
    }
    return out;
}
}  // namespace

struct BookingService::Impl {
    /// Seat inventory of one showing; the mutex guards `booked`.
    struct Show {
        std::mutex mutex;
        Mask booked = 0;
    };
    using Key = std::pair<MovieId, TheaterId>;

    // Everything except Show::booked is immutable after construction (D13), so it can be
    // read from any thread without locking. std::map nodes never move, so the Show
    // (which holds a non-movable mutex) is constructed in place.
    std::vector<Movie> movies;
    std::vector<Theater> theaters;
    std::set<MovieId> movieIds;
    std::set<TheaterId> theaterIds;
    std::map<Key, Show> shows;
    std::atomic<BookingId> nextBookingId{1};  // D14

    Show* find(MovieId m, TheaterId t) {
        auto it = shows.find({m, t});
        return it == shows.end() ? nullptr : &it->second;
    }
};

BookingService::BookingService(std::vector<Movie> movies,
                               std::vector<Theater> theaters,
                               std::vector<Showing> showings)
    : impl_(new Impl) {
    for (const auto& m : movies) {
        if (!impl_->movieIds.insert(m.id).second) throw std::invalid_argument("duplicate movie id");
    }
    for (const auto& t : theaters) {
        if (!impl_->theaterIds.insert(t.id).second) throw std::invalid_argument("duplicate theater id");
    }
    for (const auto& s : showings) {
        if (!impl_->movieIds.count(s.movieId)) throw std::invalid_argument("showing: unknown movie");
        if (!impl_->theaterIds.count(s.theaterId)) throw std::invalid_argument("showing: unknown theater");
        const bool added = impl_->shows
                               .emplace(std::piecewise_construct,
                                        std::forward_as_tuple(s.movieId, s.theaterId),
                                        std::forward_as_tuple())
                               .second;
        if (!added) throw std::invalid_argument("duplicate showing");
    }
    impl_->movies = std::move(movies);
    impl_->theaters = std::move(theaters);
}

BookingService::~BookingService() = default;

std::vector<Movie> BookingService::listMovies() const {
    return impl_->movies;
}

std::vector<Theater> BookingService::listTheaters(MovieId movie) const {
    std::vector<Theater> out;
    for (const auto& t : impl_->theaters) {
        if (impl_->shows.count({movie, t.id})) out.push_back(t);
    }
    return out;
}

std::optional<std::vector<std::string>> BookingService::availableSeats(MovieId movie,
                                                                       TheaterId theater) const {
    Impl::Show* show = impl_->find(movie, theater);
    if (!show) return std::nullopt;
    Mask booked;
    {
        std::lock_guard<std::mutex> lock(show->mutex);
        booked = show->booked;
    }
    return labelsOf(~booked & kAllSeats);  // formatted outside the lock
}

BookingResult BookingService::book(MovieId movie, TheaterId theater,
                                   const std::vector<std::string>& seats) {
    BookingResult result;

    // Validation order is part of the public contract (see header).
    if (seats.empty()) {
        result.status = BookingStatus::EmptyRequest;
        return result;
    }
    if (!impl_->movieIds.count(movie)) {
        result.status = BookingStatus::UnknownMovie;
        return result;
    }
    if (!impl_->theaterIds.count(theater)) {
        result.status = BookingStatus::UnknownTheater;
        return result;
    }
    Impl::Show* show = impl_->find(movie, theater);
    if (!show) {
        result.status = BookingStatus::NotShowing;
        return result;
    }

    // Parse and validate without holding the lock, building the requested-seat mask.
    Mask wanted = 0, duplicates = 0;
    for (const auto& label : seats) {
        const auto index = parseSeat(label);
        if (!index) {
            result.seats.push_back(label);  // collect every offending raw label
            continue;
        }
        if (wanted & bit(*index)) duplicates |= bit(*index);
        wanted |= bit(*index);
    }
    if (!result.seats.empty()) {
        result.status = BookingStatus::InvalidSeat;
        return result;
    }
    if (duplicates) {
        result.status = BookingStatus::DuplicateSeat;  // D8
        result.seats = labelsOf(duplicates);
        return result;
    }

    // Everything that can allocate (and so throw) is done BEFORE the commit, so an exception
    // can never leave seats blocked with no booking returned (F11). The critical section
    // below only does arithmetic and moves already-built objects, none of which can throw.
    std::vector<std::string> wantedLabels = labelsOf(wanted);

    // Critical section: check-and-set is ONE atomic step, which is what prevents
    // over-booking (F7). All requested seats are committed together (D7).
    Mask conflicts;
    BookingId id = 0;
    {
        std::lock_guard<std::mutex> lock(show->mutex);
        conflicts = wanted & show->booked;
        if (!conflicts) {
            show->booked |= wanted;
            id = impl_->nextBookingId++;  // ids are only consumed by successful bookings
        }
    }

    // Lock released. A failure still has to name the taken seats; that allocates, but nothing
    // was committed on this path, so a throw here leaves the show untouched.
    if (conflicts) {
        result.status = BookingStatus::SeatsUnavailable;
        result.seats = labelsOf(conflicts);
    } else {
        result.status = BookingStatus::Ok;
        result.bookingId = id;
        result.seats = std::move(wantedLabels);  // a move cannot throw
    }
    return result;
}

}  // namespace cinema
