# UML Architecture and Design Diagrams

This document provides a complete set of Unified Modeling Language (UML) diagrams for the Movie Ticket Booking system. It details the system's structural components, runtime object models, behavioral interactions, concurrency models, and state transitions.

---

## 1. System Use Case Diagram

The use case diagram illustrates the actors interacting with the system and the core capabilities provided by the [`cinema::BookingService`](include/cinema/booking_service.hpp:38) backend and [`cinema::runCli`](include/cinema/cli.hpp:29) interface.

```mermaid
flowchart TD
    Customer(("👤 Customer / User"))
    HostApp(("💻 Host App / main()"))

    subgraph Boundary [" Cinema Booking System Boundary "]
        UC1(["Browse Now Playing Movies"])
        UC2(["View Theaters Showing Movie"])
        UC3(["View Available Seats for Showing"])
        UC4(["Book Specific Seats (Atomic)"])
        UC5(["Navigate Back / Change Selection"])
        UC6(["Quit Interactive Session"])
        UC7(["Initialize Static Catalogue"])
    end

    Customer --> UC1
    Customer --> UC2
    Customer --> UC3
    Customer --> UC4
    Customer --> UC5
    Customer --> UC6

    HostApp --> UC7

    UC4 -.->|includes| UC3
    UC4 -.->|validates| UC8(["Validate Seat Labels & Availability"])
```

---

## 2. Package and Component Diagram

This diagram shows the modular structure, headers, compilation units, static library dependencies, and executables built via CMake.

```mermaid
flowchart TB
    subgraph ClientLayer [" Applications / Executables "]
        AppMain["app/main.cpp\n(Target: cinema_cli)"]
        TestRunner["tests/tests.cpp\n(Target: cinema_tests)"]
    end

    subgraph CinemaLib [" Static Library: cinema "]
        subgraph CLIModule [" CLI Subsystem "]
            CLIHeader["include/cinema/cli.hpp"]
            CLISource["src/cli.cpp"]
        end

        subgraph CoreEngine [" Core Booking Engine "]
            BSHeader["include/cinema/booking_service.hpp"]
            BSSource["src/booking_service.cpp"]
        end

        subgraph DomainCodec [" Seat Codec Subsystem "]
            SeatHeader["include/cinema/seat.hpp"]
            SeatSource["src/seat.cpp"]
        end

        subgraph TypesModule [" Common Value Types "]
            TypesHeader["include/cinema/types.hpp"]
        end
    end

    AppMain --> CLIHeader
    AppMain --> BSHeader
    TestRunner --> BSHeader
    TestRunner --> SeatHeader
    TestRunner --> TypesHeader

    CLIHeader --> BSHeader
    CLISource --> CLIHeader
    CLISource --> TypesHeader

    BSHeader --> TypesHeader
    BSSource --> BSHeader
    BSSource --> SeatHeader

    SeatSource --> SeatHeader
```

---

## 3. Class Diagram: Public API and Domain Model

This diagram documents the public data structures, enumerations, classes, and relationships exposed by [`include/cinema/types.hpp`](include/cinema/types.hpp:14), [`include/cinema/seat.hpp`](include/cinema/seat.hpp:19), and [`include/cinema/booking_service.hpp`](include/cinema/booking_service.hpp:28).

```mermaid
classDiagram
    direction TB

    class MovieId {
        <<typedef>>
        +uint32_t
    }

    class TheaterId {
        <<typedef>>
        +uint32_t
    }

    class BookingId {
        <<typedef>>
        +uint64_t
    }

    class Movie {
        +MovieId id
        +string title
    }

    class Theater {
        +TheaterId id
        +string name
    }

    class Showing {
        +MovieId movieId
        +TheaterId theaterId
    }

    class BookingStatus {
        <<enumeration>>
        Ok
        EmptyRequest
        UnknownMovie
        UnknownTheater
        NotShowing
        InvalidSeat
        DuplicateSeat
        SeatsUnavailable
    }

    class BookingResult {
        +BookingStatus status
        +BookingId bookingId
        +vector~string~ seats
        +ok() bool
    }

    class SeatCodec {
        <<utility / namespace>>
        +kRows : size_t = 4
        +kSeatsPerRow : size_t = 5
        +kSeatCount : size_t = 20
        +seatLabel(size_t index) string
        +parseSeat(string label) optional~size_t~
    }

    class BookingService {
        -unique_ptr~Impl~ impl_
        +BookingService(vector~Movie~, vector~Theater~, vector~Showing~)
        +~BookingService()
        +listMovies() vector~Movie~
        +listTheaters(MovieId) vector~Theater~
        +availableSeats(MovieId, TheaterId) optional~vector~string~~
        +book(MovieId, TheaterId, vector~string~) BookingResult
        -BookingService(BookingService&) = delete
        -operator=(BookingService&) = delete
    }

    Showing --> MovieId : references
    Showing --> TheaterId : references
    BookingResult --> BookingStatus : contains
    BookingResult --> BookingId : contains
    BookingService ..> Movie : returns
    BookingService ..> Theater : returns
    BookingService ..> Showing : initialized with
    BookingService ..> BookingResult : produces
    BookingService ..> SeatCodec : uses for conversion
```

---

## 4. Detailed Design Class Diagram (PIMPL & Concurrency Internals)

This diagram details the internal compilation-firewall implementation (`BookingService::Impl`), private data structures, container types, and fine-grained mutex synchronization.

```mermaid
classDiagram
    direction TB

    class BookingService {
        -unique_ptr~Impl~ impl_
        +BookingService(vector~Movie~, vector~Theater~, vector~Showing~)
        +~BookingService()
        +listMovies() const vector~Movie~
        +listTheaters(MovieId) const vector~Theater~
        +availableSeats(MovieId, TheaterId) const optional~vector~string~~
        +book(MovieId, TheaterId, vector~string~) BookingResult
    }

    class Impl {
        +vector~Movie~ movies
        +vector~Theater~ theaters
        +set~MovieId~ movieIds
        +set~TheaterId~ theaterIds
        +map~pair~MovieId, TheaterId~, Show~ shows
        +atomic~BookingId~ nextBookingId
        +find(MovieId, TheaterId) Show*
    }

    class Show {
        +mutex mutex
        +Mask booked
    }

    class Mask {
        <<typedef uint32_t>>
        +bit 0..19 : Seat State (1=Booked, 0=Free)
        +bit 20..31 : Unused
    }

    BookingService *-- Impl : owns (PIMPL)
    Impl *-- Show : contains per showing
    Show *-- Mask : protects via mutex
```

---

## 5. Object Diagram (Runtime Instance Snapshot)

This diagram visualizes a concrete runtime instance snapshot initialized by [`app/main.cpp`](app/main.cpp:8) with 3 movies, 3 theaters, and 5 showings.

```mermaid
flowchart TB
    subgraph BS [" BookingService instance (Singleton/Host Scope) "]
        subgraph ImplInstance [" BookingService::Impl "]
            AtomicID["nextBookingId = 1 (atomic)"]

            subgraph Catalogs [" Immutable Catalogue Sets "]
                MSet["movieIds = {1, 2, 3}"]
                TSet["theaterIds = {1, 2, 3}"]
                MLst["movies = [ {1, 'The Matrix'}, {2, 'Inception'}, {3, 'Interstellar'} ]"]
                TLst["theaters = [ {1, 'Downtown Cinema'}, {2, 'Riverside Multiplex'}, {3, 'Hilltop IMAX'} ]"]
            end

            subgraph ShowsMap [" shows: std::map&lt;pair&lt;MovieId, TheaterId&gt;, Show&gt; "]
                S1["Key: (1, 1)\nShow: { mutex, booked: 0x00000000 }"]
                S2["Key: (1, 2)\nShow: { mutex, booked: 0x00000000 }"]
                S3["Key: (2, 2)\nShow: { mutex, booked: 0x00000000 }"]
                S4["Key: (2, 3)\nShow: { mutex, booked: 0x00000000 }"]
                S5["Key: (3, 3)\nShow: { mutex, booked: 0x00000000 }"]
            end
        end
    end
```

---

## 6. Sequence Diagrams

### 6.1. Successful Seat Booking Flow

Demonstrates the execution path for [`BookingService::book()`](src/booking_service.cpp:121) from customer CLI input to atomic bitmask commit and receipt generation.

```mermaid
sequenceDiagram
    autonumber
    actor User as User / Client
    participant CLI as cinema::runCli
    participant BS as BookingService
    participant Impl as BookingService::Impl
    participant Codec as cinema::SeatCodec
    participant Show as Impl::Show (Mutex & Mask)

    User->>CLI: Input: "a1, a2"
    CLI->>BS: book(movieId=1, theaterId=1, {"a1", "a2"})
    
    rect rgb(240, 248, 255)
        note over BS: Stage 1: Fast Immutable Validation (Lock-Free)
        BS->>Impl: Check movieIds.count(1) & theaterIds.count(1)
        Impl-->>BS: Valid
        BS->>Impl: find(1, 1)
        Impl-->>BS: Pointer to Show (1,1)
    end

    rect rgb(245, 255, 245)
        note over BS, Codec: Stage 2: Parse Labels to Bitmask (Lock-Free)
        loop For each label in {"a1", "a2"}
            BS->>Codec: parseSeat("a1") -> index 0 (bit 0)
            BS->>Codec: parseSeat("a2") -> index 1 (bit 1)
        end
        note over BS: wanted = 0x00000003, duplicates = 0
    end

    rect rgb(255, 245, 240)
        note over BS, Show: Stage 3: Critical Section (Atomic Check-and-Set)
        BS->>Show: lock_guard(show.mutex)
        activate Show
        BS->>Show: conflicts = wanted & show.booked
        note over Show: conflicts == 0 (Seats are free)
        BS->>Show: show.booked |= wanted (Set bits 0 & 1)
        BS->>Impl: nextBookingId.fetch_add(1) -> Returns 1
        Show-->>BS: Mutex auto-released (RAII)
        deactivate Show
    end

    rect rgb(240, 248, 255)
        note over BS, CLI: Stage 4: Result Assembly (Lock-Free)
        BS-->>CLI: BookingResult{ status: Ok, bookingId: 1, seats: ["a1", "a2"] }
    end

    CLI-->>User: Display: "*** Booking confirmed #1: a1, a2 ***"
```

---

### 6.2. Concurrent Booking Contention (Thread Race Condition)

Illustrates how two concurrent threads attempting to book overlapping seats for the same showing are synchronized via [`std::mutex`](src/booking_service.cpp:175), ensuring zero over-booking.

```mermaid
sequenceDiagram
    autonumber
    actor ThreadA as Thread A (Wants "a1", "a2")
    actor ThreadB as Thread B (Wants "a2", "a3")
    participant Show as Show::mutex & booked mask
    participant AtomicID as Impl::nextBookingId

    par Thread A and Thread B request simultaneously
        ThreadA->>Show: Acquire mutex lock
        activate Show
        note over Show: Thread A wins lock acquisition
        ThreadB->>Show: Acquire mutex lock (BLOCKED)
    and
        note over Show: Thread A checks: wanted(a1, a2) & booked(0) == 0
        Show->>Show: booked |= wanted (a1, a2 marked booked)
        Show->>AtomicID: fetch_add(1) -> returns Booking #101
        Show-->>ThreadA: Release lock -> BookingResult::Ok (#101, ["a1", "a2"])
        deactivate Show
        
        activate Show
        note over Show: Thread B unblocks & acquires lock
        note over Show: Thread B checks: wanted(a2, a3) & booked(a1, a2) -> conflict on "a2"
        note over Show: No bits modified! booked remains unchanged
        Show-->>ThreadB: Release lock -> BookingResult::SeatsUnavailable (conflicts=["a2"])
        deactivate Show
    end
```

---

### 6.3. Information Query Sequence (Movies, Theaters, Available Seats)

Shows how clients query movies, venues, and layout seat availability without holding broad locks.

```mermaid
sequenceDiagram
    autonumber
    actor User as User
    participant CLI as cinema::runCli
    participant BS as BookingService
    participant Impl as BookingService::Impl
    participant Show as Impl::Show

    User->>CLI: Launch / Refresh Menu
    CLI->>BS: listMovies()
    BS->>Impl: Read impl_->movies (Immutable)
    Impl-->>BS: vector<Movie>
    BS-->>CLI: Return movies
    CLI-->>User: Show Movie List

    User->>CLI: Select Movie 1
    CLI->>BS: listTheaters(1)
    BS->>Impl: Iterate impl_->theaters filtering by shows.count({1, t.id})
    Impl-->>BS: vector<Theater>
    BS-->>CLI: Return matching theaters
    CLI-->>User: Show Theater List

    User->>CLI: Select Theater 2
    CLI->>BS: availableSeats(1, 2)
    BS->>Impl: find(1, 2)
    Impl-->>BS: Show*
    BS->>Show: lock_guard(mutex) -> copy show.booked
    Show-->>BS: booked mask (Lock released immediately)
    note over BS: Format labels: labelsOf(~booked & kAllSeats)
    BS-->>CLI: optional<vector<string>> { "a1", "a2", ..., "d5" }
    CLI-->>User: Display Available Seats Grid
```

---

## 7. Activity Diagrams / Flowcharts

### 7.1. Booking Validation Pipeline & Atomic Execution

Details the exact step-by-step decision tree implemented in [`BookingService::book()`](src/booking_service.cpp:121).

```mermaid
flowchart TD
    Start([Call BookingService::book]) --> CheckEmpty{seats.empty?}
    
    CheckEmpty -- Yes --> RetEmpty[Return BookingStatus::EmptyRequest]
    CheckEmpty -- No --> CheckMovie{movieIds.count(movie)?}
    
    CheckMovie -- No --> RetMovie[Return BookingStatus::UnknownMovie]
    CheckMovie -- Yes --> CheckTheater{theaterIds.count(theater)?}
    
    CheckTheater -- No --> RetTheater[Return BookingStatus::UnknownTheater]
    CheckTheater -- Yes --> CheckShowing{find(movie, theater) != nullptr?}
    
    CheckShowing -- No --> RetNotShowing[Return BookingStatus::NotShowing]
    CheckShowing -- Yes --> ParseSeats[Parse and Validate Seat Labels]

    ParseSeats --> CheckMalformed{Any parse errors?}
    CheckMalformed -- Yes --> RetInvalid[Return BookingStatus::InvalidSeat\nwith malformed strings]
    CheckMalformed -- No --> CheckDuplicates{duplicates != 0?}

    CheckDuplicates -- Yes --> RetDup[Return BookingStatus::DuplicateSeat\nwith duplicated labels]
    CheckDuplicates -- No --> PrepAlloc[Allocate wantedLabels vector\nSafe from bad_alloc]

    PrepAlloc --> AcquireLock[Acquire Show::mutex Lock]
    AcquireLock --> CheckConflict{wanted & show.booked != 0?}

    CheckConflict -- Yes --> ReleaseLockFail[Release Lock]
    ReleaseLockFail --> RetUnavailable[Return BookingStatus::SeatsUnavailable\nwith conflicting labels]

    CheckConflict -- No --> Commit[show.booked |= wanted\nid = nextBookingId++]
    Commit --> ReleaseLockSuccess[Release Lock]
    ReleaseLockSuccess --> RetOk[Return BookingStatus::Ok\nwith bookingId & seats]

    RetEmpty --> End([End])
    RetMovie --> End
    RetTheater --> End
    RetNotShowing --> End
    RetInvalid --> End
    RetDup --> End
    RetUnavailable --> End
    RetOk --> End
```

---

### 7.2. CLI Interactive Navigation Flow

Illustrates the menu navigation lifecycle in [`cinema::runCli()`](src/cli.cpp:40).

```mermaid
flowchart TD
    StartCLI([Start CLI]) --> MenuMovies[Display 'Now Playing' Movies]
    
    MenuMovies --> PromptMovie[Prompt: Movie ID / 'q' to quit]
    PromptMovie --> ReadMovieLine[/Read line from istream/]
    
    ReadMovieLine --> CheckQuit1{Input 'q' / EOF?}
    CheckQuit1 -- Yes --> ExitCLI([Exit & Display 'Goodbye'])
    CheckQuit1 -- No --> ValidateMovie{Valid Movie ID?}
    
    ValidateMovie -- No --> ErrMovie[Display 'Please enter one of the listed movie ids'] --> MenuMovies
    ValidateMovie -- Yes --> MenuTheaters[Display Theaters for Selected Movie]
    
    MenuTheaters --> PromptTheater[Prompt: Theater ID / 'b' back / 'q' quit]
    PromptTheater --> ReadTheaterLine[/Read line from istream/]
    
    ReadTheaterLine --> CheckQuit2{Input 'q' / EOF?}
    CheckQuit2 -- Yes --> ExitCLI
    CheckQuit2 -- No --> CheckBack2{Input 'b'?}
    CheckBack2 -- Yes --> MenuMovies
    CheckBack2 -- No --> ValidateTheater{Valid Theater ID?}
    
    ValidateTheater -- No --> ErrTheater[Display 'Please enter one of the listed theater ids'] --> MenuTheaters
    ValidateTheater -- Yes --> MenuSeats[Display Available Seats for Showing]
    
    MenuSeats --> PromptSeats[Prompt: Seat labels e.g. 'a1 a2' / 'b' back / 'q' quit]
    PromptSeats --> ReadSeatsLine[/Read line from istream/]
    
    ReadSeatsLine --> CheckQuit3{Input 'q' / EOF?}
    CheckQuit3 -- Yes --> ExitCLI
    CheckQuit3 -- No --> CheckBack3{Input 'b'?}
    CheckBack3 -- Yes --> MenuTheaters
    CheckBack3 -- No --> CallBook[Call BookingService::book]
    
    CallBook --> DisplayResult[Display Booking Status & Result]
    DisplayResult --> MenuSeats
```

---

## 8. State Machine Diagrams

### 8.1. Seat Lifecycle State Machine

Represents the state of an individual seat bit within a specific [`Showing`](include/cinema/types.hpp:36).

```mermaid
stateDiagram-v2
    [*] --> Free : Showing Initialized (Bit = 0)
    
    Free --> Free : availableSeats() read (~booked & bit)
    Free --> Booked : Atomic book() success (booked |= bit)
    
    Booked --> Booked : Concurrent book() rejected (conflicts & bit)
    Booked --> Booked : availableSeats() excludes seat
```

---

### 8.2. CLI Navigation State Machine

Models the finite state machine transitions of the user interface level ([`enum Level`](src/cli.cpp:41)).

```mermaid
stateDiagram-v2
    [*] --> MoviesLevel : Launch CLI

    state MoviesLevel {
        [*] --> DisplayMovies
        DisplayMovies --> AwaitMovieInput
    }

    state TheatersLevel {
        [*] --> DisplayTheaters
        DisplayTheaters --> AwaitTheaterInput
    }

    state SeatsLevel {
        [*] --> DisplaySeats
        DisplaySeats --> AwaitSeatBooking
    }

    MoviesLevel --> TheatersLevel : Select valid Movie ID
    TheatersLevel --> MoviesLevel : Command 'b' (Back)
    TheatersLevel --> SeatsLevel : Select valid Theater ID
    SeatsLevel --> TheatersLevel : Command 'b' (Back)
    SeatsLevel --> SeatsLevel : Submit booking / Refresh availability

    MoviesLevel --> [*] : Command 'q' / EOF
    TheatersLevel --> [*] : Command 'q' / EOF
    SeatsLevel --> [*] : Command 'q' / EOF
```

---

## 9. Deployment and Concurrency Architecture Diagram

Visualizes the process memory structure, execution threads, thread safety boundaries, and OS stream interfaces.

```mermaid
flowchart TB
    subgraph Process [" OS Process (cinema_cli.exe / cinema_tests.exe) "]
        subgraph ConcurrencyPool [" Concurrent Worker Threads "]
            T1["Thread 1 (e.g. CLI Loop)"]
            T2["Thread 2 (Worker / Test)"]
            TN["Thread N (Concurrent Request)"]
        end

        subgraph MemorySpace [" Process Address Space "]
            subgraph ReadOnlyData [" Read-Only / Immutable Memory (No Locks Needed) "]
                CatMovies["movies: vector&lt;Movie&gt;"]
                CatTheaters["theaters: vector&lt;Theater&gt;"]
                CatShows["shows key index: map&lt;Key, Show&gt;"]
            end

            subgraph MutableSynchronized [" Synchronized State (Fine-Grained Locking) "]
                subgraph Show1 [" Show (Movie 1, Theater 1) "]
                    Mtx1["std::mutex"]
                    Mask1["Mask booked (32-bit uint)"]
                end
                subgraph Show2 [" Show (Movie 1, Theater 2) "]
                    Mtx2["std::mutex"]
                    Mask2["Mask booked (32-bit uint)"]
                end
                subgraph GlobalAtomic [" Global State "]
                    SeqID["std::atomic&lt;BookingId&gt; nextBookingId"]
                end
            end
        end

        subgraph IOStreams [" Standard I/O Subsystem "]
            Cin["std::istream (std::cin / stringstream)"]
            Cout["std::ostream (std::cout / stringstream)"]
        end
    end

    T1 <--> Cin
    T1 <--> Cout
    T1 --> ReadOnlyData
    T2 --> ReadOnlyData
    TN --> ReadOnlyData

    T1 --> Show1
    T2 --> Show2
    TN --> Show1

    T1 --> GlobalAtomic
    T2 --> GlobalAtomic
```
