#include <ncurses.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <cstdio>

using Clock = std::chrono::steady_clock;

struct Cell
{
    int row;
    int col;
    char expected;
    bool was_correct = false;
};

void init_window()
{
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    box(stdscr, 0, 0);

    if (has_colors())
    {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_GREEN, -1);
        init_pair(2, COLOR_RED, -1);
        init_pair(3, COLOR_CYAN, -1);
    }
}

void get_random_words(std::vector<std::string> &random_words, const std::vector<std::string> &word_list, const int word_count)
{
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, word_list.size() - 1);

    random_words.clear();
    random_words.reserve(word_count);

    for (int i = 0; i < word_count; i++)
    {
        random_words.push_back(word_list[dist(gen)]);
    }
}

void load_words(std::vector<std::string> &word_list, const std::string &filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Error: Could not open " << filepath << "\n";
        return;
    }

    std::string word;
    while (std::getline(file, word))
    {
        if (!word.empty() && word.back() == '\r')
        {
            word.pop_back();
        }
        if (!word.empty())
        {
            word_list.push_back(word);
        }
    }
}

void update_menu(WINDOW *menu, const std::vector<int> &options, int selected_idx)
{
    werase(menu);
    box(menu, 0, 0);

    int col = 2;
    mvwprintw(menu, 1, col, "Words: ");
    col += 7;

    for (size_t i = 0; i < options.size(); ++i)
    {
        if (static_cast<int>(i) == selected_idx)
        {
            wattron(menu, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
            mvwprintw(menu, 1, col, " %d ", options[i]);
            wattroff(menu, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
        }
        else
        {
            wattron(menu, A_DIM);
            mvwprintw(menu, 1, col, " %d ", options[i]);
            wattroff(menu, A_DIM);
        }
        col += std::to_string(options[i]).length() + 3;
    }

    // Right-aligned hint for toggling word counts
    int width = getmaxx(menu);
    const char *hint = "LEFT/RIGHT or 1-4: Change Words";
    int hint_col = width - static_cast<int>(std::string(hint).length()) - 2;
    if (hint_col > col + 2)
    {
        wattron(menu, A_DIM);
        mvwprintw(menu, 1, hint_col, "%s", hint);
        wattroff(menu, A_DIM);
    }

    wrefresh(menu);
}

void update_stats(WINDOW *stats, float wpm, float acc, double time, bool finished = false)
{
    werase(stats);
    box(stats, 0, 0);

    int width = getmaxx(stats);
    char right_buf[128];

    if (finished)
    {
        wattron(stats, COLOR_PAIR(1) | A_BOLD);
        mvwprintw(stats, 1, 2, "Done!");
        wattroff(stats, COLOR_PAIR(1) | A_BOLD);

        int len = std::snprintf(right_buf, sizeof(right_buf),
                                "WPM: %.2f | Acc: %.2f%% | Time: %.2fs | TAB/ENTER: Restart | ESC: Quit",
                                wpm, acc, time);
        int right_col = width - len - 2;
        if (right_col > 8)
        {
            mvwprintw(stats, 1, right_col, "%s", right_buf);
        }
    }
    else
    {
        mvwprintw(stats, 1, 2, "Typing...");

        int len = std::snprintf(right_buf, sizeof(right_buf),
                                "WPM: %.2f | Acc: %.2f%% | Time: %.2fs | TAB: Restart | ESC: Quit",
                                wpm, acc, time);
        int right_col = width - len - 2;
        if (right_col > 12)
        {
            mvwprintw(stats, 1, right_col, "%s", right_buf);
        }
    }

    wrefresh(stats);
}

WINDOW *create_header(const std::string &heading)
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

bool run_test(WINDOW *body, WINDOW *menu, WINDOW *stats, const std::vector<std::string> &word_list,
              const std::vector<int> &word_options, int &selected_idx)
{
    const int body_width = getmaxx(body);
    const int row_lb = 2, col_lb = 4, col_ub = body_width - 4;
    const int word_count = word_options[selected_idx];

    std::vector<std::string> body_content;
    get_random_words(body_content, word_list, word_count);

    werase(body);
    box(body, 0, 0);
    update_menu(menu, word_options, selected_idx);
    update_stats(stats, 0.0f, 0.0f, 0.0);

    int row = row_lb, col = col_lb;
    std::vector<Cell> cells;

    wattron(body, A_DIM);
    for (int i = 0; i < word_count; i++)
    {
        const std::string &current_word = body_content[i];
        if (col + static_cast<int>(current_word.length()) + 1 > col_ub)
        {
            row++;
            col = col_lb;
        }
        for (char c : current_word)
        {
            mvwaddch(body, row, col, c);
            cells.push_back({row, col, c, false});
            col++;
        }

        if (i + 1 < word_count && col < col_ub)
        {
            mvwaddch(body, row, col, ' ');
            cells.push_back({row, col, ' ', false});
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
    float wpm = 0.0f, accuracy = 0.0f;
    double elapsed_time = 0.0;

    auto handle_menu_toggle = [&](int ch) -> bool {
        int n = static_cast<int>(word_options.size());
        if (ch == KEY_LEFT)
        {
            selected_idx = (selected_idx - 1 + n) % n;
            return true;
        }
        if (ch == KEY_RIGHT)
        {
            selected_idx = (selected_idx + 1) % n;
            return true;
        }
        if (ch >= '1' && ch <= '4')
        {
            int idx = ch - '1';
            if (idx < n && idx != selected_idx)
            {
                selected_idx = idx;
                return true;
            }
        }
        return false;
    };

    int ch;
    while (cursor < cells.size())
    {
        ch = getch();

        if (ch == 27)
        {
            return false;
        }
        if (ch == '\t' || handle_menu_toggle(ch))
        {
            return true; // Immediately restart with the newly selected word count
        }

        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')
        {
            if (cursor > 0)
            {
                cursor--;
                Cell &prev = cells[cursor];
                if (prev.was_correct)
                {
                    correct_chars--;
                    prev.was_correct = false;
                }
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
            Cell &curr = cells[cursor];
            if (ch == curr.expected)
            {
                wattron(body, COLOR_PAIR(1));
                mvwaddch(body, curr.row, curr.col, curr.expected);
                wattroff(body, COLOR_PAIR(1));
                correct_chars++;
                curr.was_correct = true;
            }
            else
            {
                int attr = COLOR_PAIR(2) | (curr.expected == ' ' ? A_REVERSE : 0);
                wattron(body, attr);
                mvwaddch(body, curr.row, curr.col, curr.expected);
                wattroff(body, attr);
                curr.was_correct = false;
            }
            cursor++;

            auto now = Clock::now();
            elapsed_time = std::chrono::duration<double>(now - start_time).count();
            double minutes = elapsed_time / 60.0;

            wpm = (elapsed_time >= 0.5) ? static_cast<float>((correct_chars / 5.0) / minutes) : 0.0f;
            accuracy = (total_typed > 0) ? (correct_chars * 100.0f) / total_typed : 100.0f;

            bool finished = (cursor == cells.size());
            update_stats(stats, wpm, accuracy, elapsed_time, finished);

            if (!finished)
            {
                wmove(body, cells[cursor].row, cells[cursor].col);
            }
        }
        wrefresh(body);
    }

    // Post-test prompt: allow toggling word count, restarting, or quitting
    while (true)
    {
        ch = getch();
        if (ch == 27)
        {
            return false;
        }
        if (ch == '\t' || ch == '\n' || ch == KEY_ENTER || handle_menu_toggle(ch))
        {
            return true;
        }
    }
}

int main()
{
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

    const std::vector<int> word_options = {10, 25, 50, 100};
    int selected_idx = 0;

    WINDOW *header = create_header(heading);
    WINDOW *body = newwin(LINES - 14, COLS - 5, 5, 2);
    WINDOW *menu = newwin(3, COLS - 5, LINES - 8, 2);
    WINDOW *stats = newwin(3, COLS - 5, LINES - 5, 2);
    refresh();

    while (run_test(body, menu, stats, word_list, word_options, selected_idx))
    {
    }

    delwin(header);
    delwin(body);
    delwin(menu);
    delwin(stats);

    endwin();
    return 0;
}
