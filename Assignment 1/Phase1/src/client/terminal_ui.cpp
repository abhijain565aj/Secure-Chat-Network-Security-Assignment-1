#include "terminal_ui.hpp"
#include <algorithm>

TerminalUI::TerminalUI() : outputWindow(nullptr), promptWindow(nullptr) {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(1);

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

void TerminalUI::printOutput(const std::string& message) {
    waddnstr(outputWindow, message.c_str(), -1);
    wrefresh(outputWindow);
    refreshPrompt();
}
