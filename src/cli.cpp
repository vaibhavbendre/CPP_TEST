// Interactive CLI. See include/cinema/cli.hpp and design.md section 9.
// No business rules live here: everything is delegated to BookingService (D16).
#include "cinema/cli.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace cinema {

namespace {

/// Split a line on whitespace and commas: "a1, a2 a3" -> {a1, a2, a3}.
std::vector<std::string> tokens(std::string line) {
    std::replace(line.begin(), line.end(), ',', ' ');
    std::istringstream iss(line);
    std::vector<std::string> out;
    for (std::string t; iss >> t;) out.push_back(t);
    return out;
}

/// Parse a token that must be a plain number, e.g. "2". Returns false otherwise.
bool toId(const std::vector<std::string>& toks, std::uint32_t& id) {
    char extra;
    std::istringstream iss(toks[0]);
    return toks.size() == 1 && (iss >> id) && !(iss >> extra);
}

std::string join(const std::vector<std::string>& items) {
    std::string s;
    for (const auto& i : items) s += (s.empty() ? "" : ", ") + i;
    return s;
}

}  // namespace

void runCli(BookingService& svc, std::istream& in, std::ostream& out) {
    enum Level { Movies, Theaters, Seats } level = Movies;
    Movie movie;
    Theater theater;

    for (;;) {
        // 1) Show the current screen and its prompt.
        if (level == Movies) {
            out << "\n== Now playing ==\n";
            for (const auto& m : svc.listMovies()) out << "  " << m.id << ") " << m.title << "\n";
            out << "Select movie id, q = quit: ";
        } else if (level == Theaters) {
            out << "\n== Theaters showing " << movie.title << " ==\n";
            for (const auto& t : svc.listTheaters(movie.id)) out << "  " << t.id << ") " << t.name << "\n";
            out << "Select theater id, b = back, q = quit: ";
        } else {
            const auto free = svc.availableSeats(movie.id, theater.id);
            out << "\n== " << movie.title << " @ " << theater.name << " ==\n"
                << "Available seats: " << (free->empty() ? "(sold out)" : join(*free)) << "\n"
                << "Enter seats to book (e.g. a1 a2), b = back, q = quit: ";
        }
        out.flush();

        // 2) Read one command; EOF behaves like quit.
        std::string line;
        if (!std::getline(in, line)) break;
        const auto toks = tokens(line);
        if (toks.empty()) continue;
        if (toks[0] == "q" || toks[0] == "Q") break;
        if (level != Movies && (toks[0] == "b" || toks[0] == "B")) {
            level = static_cast<Level>(level - 1);
            continue;
        }

        // 3) Act on it.
        if (level == Seats) {
            const BookingResult r = svc.book(movie.id, theater.id, toks);
            out << "\n" << (r.ok() ? "*** " : "!!! ") << toString(r.status);
            if (r.ok()) out << " #" << r.bookingId;
            if (!r.seats.empty()) out << ": " << join(r.seats);
            out << (r.ok() ? " ***\n" : "\n");
            continue;
        }

        std::uint32_t id = 0;
        const bool numeric = toId(toks, id);
        if (level == Movies) {
            const auto movies = svc.listMovies();
            const auto it = std::find_if(movies.begin(), movies.end(),
                                         [&](const Movie& m) { return numeric && m.id == id; });
            if (it == movies.end()) {
                out << "Please enter one of the listed movie ids.\n";
            } else {
                movie = *it;
                level = Theaters;
            }
        } else {
            const auto theaters = svc.listTheaters(movie.id);
            const auto it = std::find_if(theaters.begin(), theaters.end(),
                                         [&](const Theater& t) { return numeric && t.id == id; });
            if (it == theaters.end()) {
                out << "Please enter one of the listed theater ids.\n";
            } else {
                theater = *it;
                level = Seats;
            }
        }
    }
    out << "\nGoodbye.\n";
}

}  // namespace cinema
