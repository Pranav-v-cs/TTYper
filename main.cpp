#include <ncurses.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <cstdio>
#include <algorithm>
#include <clocale>

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
    setlocale(LC_ALL, "");
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
        col += static_cast<int>(std::to_string(options[i]).length()) + 3;
    }

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

bool handle_menu_toggle(int ch, const std::vector<int> &word_options, int &selected_idx)
{
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

void draw_progress(WINDOW *body, size_t current, size_t total, int words_done, int total_words)
{
    int width = getmaxx(body);
    int bar_width = width - 16;
    if (bar_width < 10) return;

    float ratio = (total > 0) ? static_cast<float>(current) / total : 0.0f;

    int half_steps = static_cast<int>(ratio * bar_width * 2);
    int full_blocks = half_steps / 2;
    bool has_half = (half_steps % 2) == 1;

    wmove(body, 1, 2);

    wattron(body, COLOR_PAIR(3) | A_BOLD);
    for (int i = 0; i < full_blocks; ++i)
    {
        waddstr(body, "━");
    }

    int drawn = full_blocks;
    if (has_half && drawn < bar_width)
    {
        waddstr(body, "╸");
        drawn++;
    }
    wattroff(body, COLOR_PAIR(3) | A_BOLD);

    wattron(body, A_DIM);
    for (int i = drawn; i < bar_width; ++i)
    {
        waddstr(body, "─");
    }
    wattroff(body, A_DIM);

    mvwprintw(body, 1, 2 + bar_width + 1, "%3d/%-3d", words_done, total_words);
}

WINDOW *create_header(const std::string &heading)
{
    WINDOW *header = newwin(3, COLS - 5, 1, 2);
    refresh();
    wattron(header, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(header, 1, ((COLS / 2) - (static_cast<int>(heading.length()) / 2)), "%s", heading.c_str());
    box(header, 0, 0);
    wattroff(header, COLOR_PAIR(3) | A_BOLD);
    wrefresh(header);
    return header;
}

bool run_test(WINDOW *body, WINDOW *menu, WINDOW *stats, const std::vector<std::string> &word_list,
              const std::vector<int> &word_options, int &selected_idx)
{
    curs_set(1);
    const int body_height = getmaxy(body);
    const int body_width = getmaxx(body);
    const int word_count = word_options[selected_idx];

    std::vector<std::string> body_content;
    get_random_words(body_content, word_list, word_count);

    werase(body);
    box(body, 0, 0);
    update_menu(menu, word_options, selected_idx);
    update_stats(stats, 0.0f, 0.0f, 0.0);

    // Group words into ~65-char lines for centered rendering
    const int max_line_width = std::min(65, body_width - 8);
    std::vector<std::string> lines;
    std::string current_line;

    for (int i = 0; i < word_count; i++)
    {
        const std::string &word = body_content[i];
        if (current_line.empty())
        {
            current_line = word;
        }
        else if (static_cast<int>(current_line.length() + 1 + word.length()) <= max_line_width)
        {
            current_line += " " + word;
        }
        else
        {
            lines.push_back(current_line);
            current_line = word;
        }
    }
    if (!current_line.empty())
    {
        lines.push_back(current_line);
    }

    int total_lines = static_cast<int>(lines.size());
    int start_row = std::max(3, (body_height - total_lines) / 2);
    int start_col = std::max(4, (body_width - max_line_width) / 2);

    std::vector<Cell> cells;
    wattron(body, A_DIM);

    for (int r = 0; r < total_lines; r++)
    {
        int draw_row = start_row + r;
        if (draw_row >= body_height - 1)
        {
            break;
        }

        const std::string &line = lines[r];
        for (size_t c = 0; c < line.length(); c++)
        {
            int draw_col = start_col + static_cast<int>(c);
            mvwaddch(body, draw_row, draw_col, line[c]);
            cells.push_back({draw_row, draw_col, line[c], false});
        }

        if (r + 1 < total_lines)
        {
            int space_col = start_col + static_cast<int>(line.length());
            mvwaddch(body, draw_row, space_col, ' ');
            cells.push_back({draw_row, space_col, ' ', false});
        }
    }
    wattroff(body, A_DIM);

    if (cells.empty())
    {
        return false;
    }

    draw_progress(body, 0, cells.size(), 0, word_count);

    size_t cursor = 0;
    wmove(body, cells[0].row, cells[0].col);
    wrefresh(body);

    bool started = false;
    Clock::time_point start_time;
    int correct_chars = 0;
    int total_typed = 0;
    int words_done = 0;
    float wpm = 0.0f, accuracy = 0.0f;
    double elapsed_time = 0.0;

    int ch;
    while (cursor < cells.size())
    {
        ch = getch();

        if (ch == 27)
        {
            return false;
        }
        if (ch == '\t' || handle_menu_toggle(ch, word_options, selected_idx))
        {
            return true;
        }

        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')
        {
            if (cursor > 0)
            {
                cursor--;
                Cell &prev = cells[cursor];
                if (prev.expected == ' ')
                {
                    words_done--;
                }
                if (prev.was_correct)
                {
                    correct_chars--;
                    prev.was_correct = false;
                }
                wattron(body, A_DIM);
                mvwaddch(body, prev.row, prev.col, prev.expected);
                wattroff(body, A_DIM);

                draw_progress(body, cursor, cells.size(), words_done, word_count);
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

            if (curr.expected == ' ' || cursor == cells.size())
            {
                words_done++;
            }

            draw_progress(body, cursor, cells.size(), words_done, word_count);

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

    while (true)
    {
        ch = getch();
        if (ch == 27)
        {
            return false;
        }
        if (ch == '\t' || ch == '\n' || ch == KEY_ENTER || handle_menu_toggle(ch, word_options, selected_idx))
        {
            return true;
        }
    }
    curs_set(0);
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