/**
 * @file cli.hpp
 * @brief Interactive text front-end for BookingService.
 *
 * The loop reads from / writes to caller supplied streams so that it can be
 * unit-tested with std::stringstream (design.md D-ARCH-2).
 */
#ifndef CINEMA_CLI_HPP
#define CINEMA_CLI_HPP

#include <iosfwd>

#include "cinema/booking_service.hpp"

namespace cinema {

/**
 * @brief Run the interactive booking menu until the user quits or input ends.
 *
 * Flow: movies -> theaters -> seats. At every prompt `q` quits and (below the
 * movie list) `b` goes back one level. On the seat screen, type seat labels
 * separated by spaces or commas (e.g. `a1 a2, a3`) to book them.
 * All business rules are delegated to BookingService (D16).
 *
 * @param service Service to operate on.
 * @param in      Input stream (one command per line). EOF ends the loop.
 * @param out     Output stream for menus and messages.
 */
void runCli(BookingService& service, std::istream& in, std::ostream& out);

}  // namespace cinema

#endif  // CINEMA_CLI_HPP
