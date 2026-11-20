/**
 * @file types.hpp
 * @brief Plain value types shared by every part of the cinema booking API.
 *
 * Design reference: design.md section 5 (Data model), decisions D2 and D5.
 */
#ifndef CINEMA_TYPES_HPP
#define CINEMA_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace cinema {

/// Identifier of a movie (D2: small integer ids are cheap and easy to type in a UI).
using MovieId = std::uint32_t;
/// Identifier of a theater.
using TheaterId = std::uint32_t;
/// Receipt number of a successful booking. Always non-zero for a successful booking.
using BookingId = std::uint64_t;

/// A movie that is playing.
struct Movie {
    MovieId id;          ///< Unique id of the movie.
    std::string title;   ///< Human readable title.
};

/// A theater (venue). Every theater has exactly 20 seats, see seat.hpp.
struct Theater {
    TheaterId id;        ///< Unique id of the theater.
    std::string name;    ///< Human readable name.
};

/// The fact that a movie is shown in a theater. Owns an independent seat inventory.
struct Showing {
    MovieId movieId;       ///< Movie that is playing.
    TheaterId theaterId;   ///< Theater it is playing in.
};

/**
 * @brief Outcome of BookingService::book().
 *
 * Expected failures are reported through this enum instead of exceptions (D5):
 * "seat already taken" is a normal outcome when many users compete.
 */
enum class BookingStatus {
    Ok,                ///< All requested seats are now blocked for the caller.
    EmptyRequest,      ///< The seat list was empty.
    UnknownMovie,      ///< No movie with the given id exists.
    UnknownTheater,    ///< No theater with the given id exists.
    NotShowing,        ///< Movie and theater exist, but the movie is not shown there.
    InvalidSeat,       ///< At least one seat label is malformed or outside the layout.
    DuplicateSeat,     ///< The same seat appears more than once in the request.
    SeatsUnavailable   ///< At least one requested seat is already booked; nothing was blocked.
};

/// Short human readable description of a status (suitable for showing to an end user). Defined in booking_service.cpp.
const char* toString(BookingStatus status);

/**
 * @brief Result of a booking attempt.
 *
 * | status             | bookingId | seats                                         |
 * |--------------------|-----------|-----------------------------------------------|
 * | Ok                 | non-zero  | canonical labels that are now blocked         |
 * | SeatsUnavailable   | 0         | requested seats that were already taken       |
 * | InvalidSeat        | 0         | the offending raw strings                     |
 * | DuplicateSeat      | 0         | the duplicated canonical labels               |
 * | all others         | 0         | empty                                         |
 */
struct BookingResult {
    BookingStatus status = BookingStatus::EmptyRequest;  ///< Outcome.
    BookingId bookingId = 0;                             ///< Receipt id (0 unless status == Ok).
    std::vector<std::string> seats;                      ///< See table above.

    /// Convenience: true iff status == BookingStatus::Ok.
    bool ok() const { return status == BookingStatus::Ok; }
};

}  // namespace cinema

#endif  // CINEMA_TYPES_HPP
