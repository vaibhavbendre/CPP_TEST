# How to run the Movie Ticket Booking program (for everyone)

No programming knowledge is needed. This is a small text program: you type a number or a
seat name, press **Enter**, and the program answers. Nothing is installed, nothing is
sent over the internet, and all bookings are forgotten when you close the program.

---

## Part 1 - Start the program (Windows)

1. Open the folder that contains this file (`HOW_TO_RUN.md`).
2. Look for a file named **`run_cinema.bat`** and **double-click** it.
3. A black window opens and shows a list of movies. You are in!

**If Windows says "Windows protected your PC"**: click **More info**, then **Run anyway**.
This appears because the program is new to Windows; it is safe, it only prints text.

**If the window says "Could not find cinema_cli.exe"**: the program has not been built yet.
Ask the person who gave you this folder for the ready-made `cinema_cli.exe`, and put it
next to `run_cinema.bat`. (Developers: see "Part 5".)

---

## Part 2 - Book seats in 4 steps

The window always shows a short list and a question at the end. **Type your answer, press Enter.**

**Step 1 - Pick a movie.** You see:

```
== Now playing ==
  1) The Matrix
  2) Inception
  3) Interstellar
Select movie id, q = quit:
```

Type **`1`** and press Enter.

**Step 2 - Pick a cinema (theater).** You see the cinemas showing that movie:

```
  1) Downtown Cinema
  2) Riverside Multiplex
Select theater id, b = back, q = quit:
```

Type **`1`** and press Enter.

**Step 3 - Look at the free seats.** You see:

```
Available seats: a1, a2, a3, a4, a5, b1, b2, ... d5
Enter seats to book (e.g. a1 a2), b = back, q = quit:
```

Every cinema has 20 seats: rows **a, b, c, d** with seats **1 to 5** in each row. Only free seats are listed.

**Step 4 - Book.** Type the seats you want, with spaces or commas between them, e.g. **`a1 a2`**, and press Enter.

```
*** Booking confirmed #1: a1, a2 ***
```

Done. Those seats are now taken and disappear from the list.

---

## Part 3 - Keys you can use at any question

| Type | What happens |
|---|---|
| `b` then Enter | Go **back** one step (not available on the first list) |
| `q` then Enter | **Quit** the program |
| Capital or small letters | Both work: `A1` and `a1` are the same seat |

---

## Part 4 - Messages you may see, and what to do

| Message | Meaning | What to do |
|---|---|---|
| `Booking confirmed #N` | Success. `N` is your booking number | Nothing, enjoy the movie |
| `Some of the seats are already booked: a1` | Someone already has that seat | Choose other seats from the list. **None** of your seats were taken (it is all or nothing) |
| `Invalid seat label: zz` | That is not a seat name | Use letters **a-d** and numbers **1-5**, like `c3` |
| `The same seat was requested more than once` | You typed the same seat twice | Type each seat once |
| `Please enter one of the listed movie ids.` | The number is not on the list | Type a number that you can see on the screen |
| `(sold out)` | No seats left | Press `b` and pick another cinema or movie |

Pressing **Enter** on an empty line does nothing. You cannot break anything by typing something wrong.

---

## Part 5 - For the person who prepares the program (one time)

1. Install **Visual Studio 2022** (the free *Community* edition is fine) with the workload
   **"Desktop development with C++"**.
2. In this folder, double-click **`build_windows.bat`**. Wait until you see `Build OK`
   (about a minute). It also runs the self-tests; they must say `100% tests passed`.
3. Copy `build\cinema_cli.exe` next to `run_cinema.bat` if you want to hand the folder to someone else.
   The `.exe` is self-contained: it does not need Visual Studio or anything else installed on the other PC.

**Linux / macOS:** open a terminal in this folder and run

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build && ./build/cinema_cli
```

---

## Part 6 - Trying it with two people at once

Run `run_cinema.bat` twice? Each window is a separate program with its own seats, so they will **not** see each other's bookings.
The "many customers at once" protection is inside one running program (many requests in parallel); it is checked by the automatic tests in `build_windows.bat`.
