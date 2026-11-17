#include <ncurses.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>

struct Cell {
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

WINDOW *create_body(const std::vector<std::string> &word_list)
{
    std::vector<std::string> body_content;
    int word_count = 100;
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
        for (char c : current_word) {
            mvwaddch(body, row, col, c);
            cells.push_back({row, col, c});
            col++;
        }

        if (i + 1 < word_count && col < col_ub) {
            mvwaddch(body, row, col, ' ');
            cells.push_back({row, col, ' '});
            col++;
        }
    }
    wattroff(body, A_DIM);

    size_t cursor = 0;
    wmove(body, row, col);
    wrefresh(body);

    int ch;
    while ((ch = getch()) != 27 && cursor < cells.size())
    {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')
        {
            if (cursor > 0)
            {
                cursor--;
                const Cell& prev = cells[cursor];
                wattron(body, A_DIM);
                mvwaddch(body, prev.row, prev.col, prev.expected);
                wattroff(body, A_DIM);
                wmove(body, prev.row, prev.col);
            }
        }
        else if (isprint(ch))
        {
            const Cell& curr = cells[cursor];
            if (ch == curr.expected) {
                wattron(body, COLOR_PAIR(1));
                mvwaddch(body, curr.row, curr.col, curr.expected);
                wattroff(body, COLOR_PAIR(1));
            } else {
                wattron(body, COLOR_PAIR(2));
                mvwaddch(body, curr.row, curr.col, curr.expected);
                wattroff(body, COLOR_PAIR(2));
            }
            cursor++;

            if (cursor < cells.size()) {
                wmove(body, cells[cursor].row, cells[cursor].col);
            }
        }
        wrefresh(body);
    }
    return body;
}

WINDOW *create_stats()
{
    WINDOW *stats = newwin(3, COLS - 5, LINES - 5, 2);
    refresh();
    mvwprintw(stats, 1, 1, "Stats here...");
    box(stats, 0, 0);
    wrefresh(stats);
    return stats;
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

    WINDOW *header = create_header(heading);

    WINDOW *stats = create_stats();

    WINDOW *body = create_body(word_list);

    delwin(header);
    delwin(body);
    delwin(stats);

    endwin();
    return 0;
}