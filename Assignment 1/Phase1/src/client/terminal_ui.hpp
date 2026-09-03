#ifndef TERMINAL_UI_HPP
#define TERMINAL_UI_HPP

#include <ncurses.h>
#include <string>

class TerminalUI {
    WINDOW* outputWindow;
    WINDOW* promptWindow;
    std::string inputBuffer;

    void refreshPrompt();

public:
    TerminalUI();
    ~TerminalUI();

    bool readInput(std::string& message);
    void printOutput(const std::string& message);
};

#endif // TERMINAL_UI_HPP
