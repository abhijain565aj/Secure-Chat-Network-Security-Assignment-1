#include "terminal_ui.hpp"
#include <algorithm>

TerminalUI::TerminalUI() : outputWindow(nullptr), promptWindow(nullptr) {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(1);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_YELLOW, -1);
    init_pair(4, COLOR_RED, -1);

    int rows;
    int columns;
    getmaxyx(stdscr, rows, columns);
    outputWindow = newwin(rows - 2, columns, 0, 0);
    promptWindow = newwin(2, columns, rows - 2, 0);
    scrollok(outputWindow, TRUE);
    leaveok(outputWindow, TRUE);
    leaveok(promptWindow, FALSE);
    refreshPrompt();
}

TerminalUI::~TerminalUI() {
    if (outputWindow != nullptr) {
        delwin(outputWindow);
        delwin(promptWindow);
        endwin();
    }
}

void TerminalUI::refreshPrompt() {
    werase(promptWindow);
    mvwaddnstr(promptWindow, 0, 0, "> ", getmaxx(promptWindow) - 1);
    mvwaddnstr(promptWindow, 0, 2, inputBuffer.c_str(),
               getmaxx(promptWindow) - 3);
    wmove(promptWindow, 0, std::min(2 + static_cast<int>(inputBuffer.size()),
                                   getmaxx(promptWindow) - 1));
    wrefresh(promptWindow);
}

bool TerminalUI::readInput(std::string& message) {
    bool submitted = false;
    int character;

    while ((character = getch()) != ERR) {
        if (character == '\n' || character == KEY_ENTER) {
            message = inputBuffer;
            inputBuffer.clear();
            submitted = true;
            refreshPrompt();
            break;
        }
        if (character == KEY_BACKSPACE || character == 127 || character == '\b') {
            if (!inputBuffer.empty()) inputBuffer.pop_back();
        } else if (character >= 32 && character <= 126) {
            inputBuffer += static_cast<char>(character);
        }
    }

    refreshPrompt();
    return submitted;
}

void TerminalUI::printOutput(const Presentation::Output& output) {
    int colorPair = 0;
    switch (output.color) {
    case Presentation::Color::Server: colorPair = 1; break;
    case Presentation::Color::Incoming: colorPair = 2; break;
    case Presentation::Color::Own: colorPair = 3; break;
    case Presentation::Color::Error: colorPair = 4; break;
    case Presentation::Color::Normal: break;
    }
    wattron(outputWindow, COLOR_PAIR(colorPair));
    waddnstr(outputWindow, output.text.c_str(), -1);
    wattroff(outputWindow, COLOR_PAIR(colorPair));
    wrefresh(outputWindow);
    refreshPrompt();
}

void TerminalUI::printOutput(const std::string& message) {
    printOutput({message, Presentation::Color::Normal});
}
