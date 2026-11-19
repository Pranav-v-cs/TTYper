# TTYper

A terminal-based typing practice and test tool built with C++ and ncurses. Practice typing, track your speed and accuracy in real time, and retry as many times as you like — all without leaving the terminal.

## Features

- Randomly selected words from a built-in word list (`data/word_list.csv`)
- Selectable test lengths: 10, 25, 50, or 100 words
- Live WPM, accuracy, elapsed time, and word progress
- Visual feedback: correct characters in green, errors in red (mistyped spaces are highlighted)
- Progress bar with half-step granularity

## Installation

TTYper is built from source. You'll need a C++ compiler and the ncurses development library, then you can clone the repo and compile it with the included Makefile.

### 1. Install prerequisites

**macOS**

Install the Xcode Command Line Tools (provides `g++`/`clang++` and `make`):

```sh
xcode-select --install
```

If ncurses headers aren't available, install them with Homebrew:

```sh
brew install ncurses
```

**Debian / Ubuntu**

```sh
sudo apt update
sudo apt install build-essential libncurses-dev
```

**Fedora / RHEL**

```sh
sudo dnf install gcc-c++ make ncurses-devel
```

**Arch Linux**

```sh
sudo pacman -S base-devel ncurses
```

### 2. Get the source

```sh
git clone https://github.com/Pranav-v-cs/TTYper.git
cd TTYper
```

### 3. Build

```sh
mkdir -p build
make
```

This produces the executable at `build/ttyper`. To compile manually instead:

```sh
mkdir -p build
g++ main.cpp -lncurses -o build/ttyper
```

### 4. Run

```sh
make run
```

or directly:

```sh
./build/ttyper
```

> Note: the program loads words from `data/word_list.csv` using a relative path, so run it from the project root.

### Uninstall

TTYper installs no system files — just remove the checkout or the binary:

```sh
rm -rf TTYper
```

## Requirements

- A C++ compiler (e.g. `g++` or `clang++`) and `make`
- ncurses development headers and libraries (`-lncurses`)
- A terminal that supports color and Unicode box characters

## Controls

| Key | Action |
| --- | --- |
| Type characters | Type the displayed text; the timer starts on the first keystroke |
| `Backspace` | Move back one character and correct a mistake |
| `←` / `→` | Change the word count (10 / 25 / 50 / 100) |
| `1`–`4` | Select the word count directly |
| `Tab` | Restart the test |
| `Enter` | Restart after finishing a test |
| `Esc` | Quit the program |

## How Scoring Works

- **WPM** = (correct characters / 5) / elapsed minutes, counted only after the first keypress.
- **Accuracy** = (correct characters / total characters typed) × 100.
- **Time** = seconds elapsed since the first printable keystroke.

## Project Structure

```
.
├── main.cpp              # Entire application: ncurses UI, input handling, and scoring
├── Makefile              # Build (`make`) and run (`make run`) targets
├── data/
│   └── word_list.csv     # Word list used to generate tests (one word per line)
└── build/
    └── ttyper            # Compiled binary (generated, git-ignored)
```
