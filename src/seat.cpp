// Seat label codec. See include/cinema/seat.hpp and design.md D3/D4.
#include "cinema/seat.hpp"

#include <stdexcept>

namespace cinema {

std::string seatLabel(std::size_t index) {
    if (index >= kSeatCount) {
        throw std::out_of_range("seat index out of range");
    }
    return std::string{static_cast<char>('a' + index / kSeatsPerRow),
                       static_cast<char>('1' + index % kSeatsPerRow)};
}

std::optional<std::size_t> parseSeat(const std::string& label) {
    // Exactly "<row letter><single digit>": this also rejects "", "a01", "a10", "a1x", " a1".
    if (label.size() != 2) {
        return std::nullopt;
    }
    // `| 0x20` folds A-Z to a-z; every other character either stays outside a..d or is rejected below.
    const int row = (label[0] | 0x20) - 'a';
    const int number = label[1] - '1';
    if (row < 0 || row >= static_cast<int>(kRows) || number < 0 ||
        number >= static_cast<int>(kSeatsPerRow)) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(row) * kSeatsPerRow + static_cast<std::size_t>(number);
}

}  // namespace cinema
