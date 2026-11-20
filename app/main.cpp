// Entry point of the demo CLI: builds a small in-memory catalogue and runs the menu.
#include <iostream>

#include "cinema/booking_service.hpp"
#include "cinema/cli.hpp"

int main() {
    cinema::BookingService service(
        {{1, "The Matrix"}, {2, "Inception"}, {3, "Interstellar"}},
        {{1, "Downtown Cinema"}, {2, "Riverside Multiplex"}, {3, "Hilltop IMAX"}},
        // (movie, theater) pairs: Matrix in 1 and 2, Inception in 2 and 3, Interstellar in 3.
        {{1, 1}, {1, 2}, {2, 2}, {2, 3}, {3, 3}});
    cinema::runCli(service, std::cin, std::cout);
    return 0;
}
