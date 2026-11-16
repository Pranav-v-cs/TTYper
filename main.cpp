#include <ncurses.h>
#include <string>
#include <iostream>
#include<fstream>
#include<vector>


void init_window() {
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

void load_words(std::vector<std::string>& word_list, const std::string& filepath) {
    std::string word;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filepath << "\n";
        return;
    }

    while (std::getline(file, word)) {
        word_list.push_back(word);
    }    
}

WINDOW* create_header(std::string heading) {
    WINDOW *header = newwin(3, COLS - 5, 1, 2);
    refresh();
    wattron(header, COLOR_PAIR(3));
    mvwprintw(header, 1, ((COLS / 2) - (heading.length() / 2)), "%s", heading.c_str());
    box(header, 0, 0);
    wattroff(header, COLOR_PAIR(3));
    wrefresh(header);

    return header;
}

WINDOW* create_body() {
    WINDOW *body = newwin(LINES - 10, COLS - 5, 5, 2);
    refresh();
    box(body, 0, 0);
    wmove(body, 2, 2);
    wrefresh(body);

    int ch;
    int row = 5;
    int col = 5;
    while ((ch = getch()) != 27)
    {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b')
        {
            if (col > 5)
            {
                col--;
                mvwaddch(body, row, col, ' ');
                wmove(body, row, col);
            }
            else if (col == 5 && row > 5)
            {
                row--;
                col = COLS - 11;
                mvwaddch(body, row, col, ' ');
                wmove(body, row, col);
            }
        }
        else if (isprint(ch))
        {
            if (col < COLS - 10)
            {
                wattron(body, COLOR_PAIR(1));
                mvwaddch(body, row, col, ch);
                wattroff(body, COLOR_PAIR(1));
                col++;
            }
            else
            {
                row++;
                col = 5;
                wattron(body, COLOR_PAIR(1));
                mvwaddch(body, row, col++, ch);
                wattroff(body, COLOR_PAIR(1));
            }
        }
        wrefresh(body);
    }
    return body;
}

WINDOW* create_stats() {
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

    std::string filepath = "data/word_list.csv";
    std::string heading = "TTYper - Terminal Based Typing Practice and Test";

    load_words(word_list, filepath);

    if (word_list.empty()) {
        return 1;
    }

    init_window();

    WINDOW* header = create_header(heading);

    WINDOW* stats = create_stats();

    WINDOW* body = create_body();

    delwin(header);
    delwin(body);
    delwin(stats);

    endwin();
    return 0;
}