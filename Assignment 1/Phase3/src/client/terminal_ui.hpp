#ifndef TERMINAL_UI_HPP
#define TERMINAL_UI_HPP

#include <ncurses.h>
#include <string>
#include "chat/presentation.hpp"

class TerminalUI {
    WINDOW* outputWindow;
    WINDOW* promptWindow;
    std::string inputBuffer;
    bool colorsEnabled;
    int terminalRows;
    int terminalColumns;

    void refreshPrompt();
    void resizeLayout(int promptRows);
    void printDivider(const std::string& text);

public:
    TerminalUI();
    ~TerminalUI();

    bool readInput(std::string& message);
    void clearOutput();
    void printOutput(const Presentation::Output& output);
    void printOutput(const std::string& message);
};

#endif // TERMINAL_UI_HPP
