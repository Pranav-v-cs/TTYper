#include <ncurses.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>

using Clock = std::chrono::steady_clock;

struct Cell
{
    int row;
    int col;
    char expected;
};

void init_window()
{
    initscr();
    cbreak();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    box(stdscr, 0, 0);

    if (has_colors())
    {
        start_color();
        use_default_colors();
        init_pair(1, 2, -1);
        init_pair(2, 1, -1);
        init_pair(3, COLOR_BLACK, -1);
    }
}

void get_random_words(std::vector<std::string> &random_words, const std::vector<std::string> &word_list, const int word_count)
{
    std::random_device rd;
    std::mt19937 gen(rd());

    std::uniform_int_distribution<size_t> dist(0, word_list.size() - 1);
    random_words.reserve(word_count);
    size_t random_idx = 0;

    for (int i = 0; i < word_count; i++)
    {
        random_idx = dist(gen);
        random_words.push_back(word_list[random_idx]);
    }
}

void load_words(std::vector<std::string> &word_list, const std::string &filepath)
{
    std::string word;
    std::ifstream file(filepath);

    if (!file.is_open())
    {
        std::cerr << "Error: Could not open " << filepath << "\n";
        return;
    }

    while (std::getline(file, word))
    {
        word_list.push_back(word);
    }
}

void update_stats(WINDOW *stats, int time_left, int correct_chars, int elapsed_seconds)
{
    werase(stats);
    box(stats, 0, 0);

    int wpm = 0;
    if (elapsed_seconds > 0)
    {
        wpm = (correct_chars / 5) * 60 / elapsed_seconds;
    }

    mvwprintw(stats, 1, 2, "Time: %2ds  |  WPM: %3d  |  ESC to quit", time_left, wpm);
    wrefresh(stats);
}

WINDOW *create_header(std::string heading)
{
    WINDOW *header = newwin(3, COLS - 5, 1, 2);
    refresh();
    wattron(header, COLOR_PAIR(3));
    mvwprintw(header, 1, ((COLS / 2) - (heading.length() / 2)), "%s", heading.c_str());
    box(header, 0, 0);
    wattroff(header, COLOR_PAIR(3));
    wrefresh(header);

    return header;
}

WINDOW *create_body(const std::vector<std::string> &word_list, float &wpm, float &accuracy, double &elapsed_time)
{
    std::vector<std::string> body_content;
    int word_count = 10;
    get_random_words(body_content, word_list, word_count);

    const int body_height = (LINES - 10) / 2;
    const int body_width = COLS - 5;
    WINDOW *body = newwin(body_height, body_width, 5, 2);
    refresh();
    box(body, 0, 0);

    const int row_lb = 2, row_ub = body_height - 2;
    const int col_lb = 4, col_ub = body_width - 4;

    int row = row_lb, col = col_lb;

    std::vector<Cell> cells;

    wattron(body, A_DIM);
    for (int i = 0; i < word_count; i++)
    {
        std::string current_word = body_content[i];
        if (col + current_word.length() + 2 > col_ub)
        {
            row++;
            col = col_lb;
        }
        for (char c : current_word)
        {
            mvwaddch(body, row, col, c);
            cells.push_back({row, col, c});
            col++;
        }

        if (i + 1 < word_count && col < col_ub)
        {
            mvwaddch(body, row, col, ' ');
            cells.push_back({row, col, ' '});
            col++;
        }
    }
    wattroff(body, A_DIM);

    size_t cursor = 0;
    wmove(body, cells[cursor].row, cells[cursor].col);
    wrefresh(body);

    bool started = false;
    Clock::time_point start_time;
    int correct_chars = 0;
    int total_typed = 0;

    int ch;
    while ((ch = getch()) != 27 && cursor < cells.size())
    {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')
        {
            if (cursor > 0)
            {
                cursor--;
                const Cell &prev = cells[cursor];
                wattron(body, A_DIM);
                mvwaddch(body, prev.row, prev.col, prev.expected);
                wattroff(body, A_DIM);
                wmove(body, prev.row, prev.col);
            }
        }
        else if (isprint(ch))
        {
            if (!started)
            {
                started = true;
                start_time = Clock::now();
            }

            total_typed++;
            const Cell &curr = cells[cursor];
            if (ch == curr.expected)
            {
                wattron(body, COLOR_PAIR(1));
                mvwaddch(body, curr.row, curr.col, curr.expected);
                wattroff(body, COLOR_PAIR(1));
                correct_chars++;
            }
            else
            {
                wattron(body, COLOR_PAIR(2));
                mvwaddch(body, curr.row, curr.col, curr.expected);
                wattroff(body, COLOR_PAIR(2));
            }
            cursor++;

            if (cursor < cells.size())
            {
                wmove(body, cells[cursor].row, cells[cursor].col);
            }
            if (cursor == cells.size() - 1)
            {
                auto end_time = Clock::now();

                double elapsed_sec = std::chrono::duration<double>(end_time - start_time).count();
                double minutes = elapsed_sec / 60.0;

                wpm = (minutes > 0) ? static_cast<float>((correct_chars / 5.0) / minutes) : 0.0f;
                accuracy = (total_typed > 0) ? (correct_chars * 100.0f) / total_typed : 100.0f;
                elapsed_time = elapsed_sec;
            }
        }
        wrefresh(body);
    }
    return body;
}

WINDOW *create_stats(const float wpm, const float accuracy, const double time)
{
    WINDOW *stats = newwin(3, (COLS / 2), LINES - 5, 2);
    refresh();
    mvwprintw(stats, 1, 1, "WPM: %.2f  |   Accuracy: %.2f   |   Elapsed Time: %.2f", wpm, accuracy, time);
    box(stats, 0, 0);
    wrefresh(stats);
    return stats;
}

int main()
{
    float wpm = 0.0f, accuracy = 0.0f;
    double elapsed_time = 0.0f;
    std::vector<std::string> word_list;
    word_list.reserve(1000);

    const std::string filepath = "data/word_list.csv";
    const std::string heading = "TTYper - Terminal Based Typing Practice and Test";

    load_words(word_list, filepath);

    if (word_list.empty())
    {
        return 1;
    }

    init_window();

    WINDOW *header = create_header(heading);

    WINDOW *stats = create_stats(wpm, accuracy, elapsed_time);

    WINDOW *body = create_body(word_list, wpm, accuracy, elapsed_time);

    stats = create_stats(wpm, accuracy, elapsed_time);

    getch();

    delwin(header);
    delwin(body);
    delwin(stats);

    endwin();
    return 0;
}