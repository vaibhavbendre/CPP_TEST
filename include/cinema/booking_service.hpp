/**
 * @file booking_service.hpp
 * @brief Public API of the movie ticket booking backend.
 *
 * Typical use:
 * @code
 * cinema::BookingService svc(movies, theaters, showings);
 * for (const auto& m : svc.listMovies()) { ... }                 // view movies
 * auto theaters = svc.listTheaters(movieId);                     // theaters for a movie
 * auto free     = svc.availableSeats(movieId, theaterId);        // optional<vector<string>>
 * auto result   = svc.book(movieId, theaterId, {"a1", "a2"});   // atomic booking
 * if (result.ok()) { ... result.bookingId ... }
 * @endcode
 *
 * Thread safety: every member function may be called concurrently from any number
 * of threads on the same object (design.md section 8).
 */
#ifndef CINEMA_BOOKING_SERVICE_HPP
#define CINEMA_BOOKING_SERVICE_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "cinema/types.hpp"

namespace cinema {

/**
 * @brief In-memory, thread-safe movie ticket booking service.
 *
 * The catalogue (movies, theaters, showings) is fixed at construction (design.md
 * assumption A2). Each showing - a (movie, theater) pair - owns an independent set of
 * 20 seats. The class is non-copyable (D10) and uses the PIMPL idiom to keep
 * synchronisation details out of the public header.
 */
class BookingService {
public:
    /**
     * @brief Create a service with a fixed catalogue; all seats start free.
     *
     * @param movies   Movies that are playing. Ids must be unique.
     * @param theaters Theaters. Ids must be unique.
     * @param showings Which movie plays in which theater. Each pair at most once;
     *                 both ids must exist in @p movies / @p theaters.
     * @throws std::invalid_argument if any of the rules above is violated.
     */
    BookingService(std::vector<Movie> movies,
                   std::vector<Theater> theaters,
                   std::vector<Showing> showings);
    ~BookingService();

    BookingService(const BookingService&) = delete;
    BookingService& operator=(const BookingService&) = delete;

    /// All playing movies, in the order they were supplied to the constructor.
    std::vector<Movie> listMovies() const;

    /**
     * @brief Theaters that show the given movie, in the order theaters were supplied.
     * @return Empty vector if the movie is unknown or is not shown anywhere.
     */
    std::vector<Theater> listTheaters(MovieId movie) const;

    /**
     * @brief Labels of the seats that are still free for a movie in a theater.
     * @return Labels in layout order (e.g. a1, a3, b2), possibly empty if sold out;
     *         std::nullopt if the movie is not shown in that theater (or an id is unknown).
     */
    std::optional<std::vector<std::string>> availableSeats(MovieId movie, TheaterId theater) const;

    /**
     * @brief Atomically block one or more seats ("book tickets").
     *
     * All-or-nothing: if any requested seat is unavailable, no seat is blocked.
     * Under concurrent calls a seat is granted to at most one caller (no over-booking).
     * Seat labels are case-insensitive ("A1" == "a1").
     *
     * Checks are performed in this order and the first failure is reported:
     * empty request, unknown movie, unknown theater, movie not shown in theater,
     * invalid seat label, duplicate seat, seat already booked.
     *
     * @param movie   Movie id.
     * @param theater Theater id.
     * @param seats   Seat labels to block (at least one).
     * @return See BookingResult. Never throws for bad input.
     *
     * Exception safety: the only possible exception is `std::bad_alloc` (out of memory).
     * It is never thrown after seats are blocked, so a call that throws has blocked nothing
     * and may simply be retried.
     */
    BookingResult book(MovieId movie, TheaterId theater, const std::vector<std::string>& seats);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace cinema

#endif  // CINEMA_BOOKING_SERVICE_HPP
