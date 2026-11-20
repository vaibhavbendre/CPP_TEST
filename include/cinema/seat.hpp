/**
 * @file seat.hpp
 * @brief Seat layout and label <-> index conversion.
 *
 * Every theater has 20 seats arranged as 4 rows (a-d) of 5 seats (1-5).
 * Labels are `a1 .. a5, b1 .. b5, c1 .. c5, d1 .. d5`.
 *
 * Design reference: design.md decisions D3 and D4. The layout is defined by kRows and
 * kSeatsPerRow only. Limits: at most 32 seats (state is a 32-bit mask), at most 9 seats
 * per row (a seat number is one digit) and at most 26 rows.
 */
#ifndef CINEMA_SEAT_HPP
#define CINEMA_SEAT_HPP

#include <cstddef>
#include <optional>
#include <string>

namespace cinema {

constexpr std::size_t kRows = 4;                            ///< Rows 'a'..'d'.
constexpr std::size_t kSeatsPerRow = 5;                     ///< Seats 1..5 in each row.
constexpr std::size_t kSeatCount = kRows * kSeatsPerRow;    ///< 20 seats per theater.
static_assert(kSeatCount <= 32, "seat state is stored in a 32-bit mask");
static_assert(kSeatsPerRow <= 9 && kRows <= 26, "labels are one letter plus one digit");

/**
 * @brief Canonical (lower-case) label of the seat with the given index.
 * @param index Seat index in [0, kSeatCount).
 * @throws std::out_of_range if index is outside the layout.
 */
std::string seatLabel(std::size_t index);

/**
 * @brief Parse a user supplied seat label.
 *
 * Strict: exactly one row letter followed by one digit. The letter is case-insensitive
 * ("A1" == "a1"). Whitespace is NOT trimmed; callers tokenize first.
 *
 * @return seat index in [0, kSeatCount), or std::nullopt if the label is invalid.
 */
std::optional<std::size_t> parseSeat(const std::string& label);

}  // namespace cinema

#endif  // CINEMA_SEAT_HPP
