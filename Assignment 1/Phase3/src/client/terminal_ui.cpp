#include "terminal_ui.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

TerminalUI::TerminalUI() : outputWindow(nullptr), promptWindow(nullptr),
    colorsEnabled(false), terminalRows(0), terminalColumns(0) {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(1);
    if (has_colors() && start_color() != ERR) {
        use_default_colors();
        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_WHITE, -1);
        init_pair(3, COLOR_GREEN, -1);
        init_pair(4, COLOR_YELLOW, -1);
        init_pair(5, COLOR_RED, -1);
        colorsEnabled = true;
    }

    getmaxyx(stdscr, terminalRows, terminalColumns);
    const int outputRows = std::max(1, terminalRows - 2);
    outputWindow = newwin(outputRows, terminalColumns, 0, 0);
    promptWindow = newwin(terminalRows - outputRows, terminalColumns,
                          outputRows, 0);
    if (outputWindow == nullptr || promptWindow == nullptr) {
        if (outputWindow != nullptr) delwin(outputWindow);
        if (promptWindow != nullptr) delwin(promptWindow);
        outputWindow = nullptr;
        promptWindow = nullptr;
        endwin();
        throw std::runtime_error("Unable to create terminal windows");
    }
    scrollok(outputWindow, TRUE);
    leaveok(outputWindow, TRUE);
    leaveok(promptWindow, FALSE);
    refreshPrompt();
}

void TerminalUI::resizeLayout(int promptRows) {
    promptRows = std::min(promptRows, std::max(2, terminalRows - 1));
    const int outputRows = std::max(1, terminalRows - promptRows);
    if (getmaxy(outputWindow) == outputRows &&
        getmaxy(promptWindow) == promptRows) {
        return;
    }

    wresize(outputWindow, outputRows, terminalColumns);
    mvwin(promptWindow, outputRows, 0);
    wresize(promptWindow, promptRows, terminalColumns);
    touchwin(outputWindow);
}

TerminalUI::~TerminalUI() {
    if (outputWindow != nullptr) {
        delwin(outputWindow);
        delwin(promptWindow);
        endwin();
        if (!outputHistory.empty()) {
            std::cerr << "\r\n--- Client session log ---\r\n";
            for (const auto& message : outputHistory) {
                for (char character : message) {
                    if (character == '\n') std::cerr << '\r';
                    std::cerr << character;
                }
            }
            std::cerr << "\r\n--- End client session log ---\r\n";
        }
    }
}

void TerminalUI::refreshPrompt() {
    const int inputWidth = std::max(1, terminalColumns - 10);
    const int inputRows = std::max(1,
        (static_cast<int>(inputBuffer.size()) + inputWidth - 1) / inputWidth);
    resizeLayout(1 + inputRows);

    werase(promptWindow);
    const int width = getmaxx(promptWindow);
    const int promptRows = getmaxy(promptWindow);
    if (promptRows == 0 || width == 0) return;

    if (colorsEnabled) wattron(promptWindow, COLOR_PAIR(1));
    wattron(promptWindow, A_DIM);
    mvwhline(promptWindow, 0, 0, ACS_HLINE, width);
    wattroff(promptWindow, A_DIM);
    if (colorsEnabled) wattroff(promptWindow, COLOR_PAIR(1));

    if (promptRows < 2 || width < 2) {
        refreshScreen();
        return;
    }

    const std::string prompt = "Command > ";
    const int inputColumn = std::min(static_cast<int>(prompt.size()), width - 1);
    if (colorsEnabled) wattron(promptWindow, COLOR_PAIR(3));
    wattron(promptWindow, A_BOLD);
    mvwaddnstr(promptWindow, 1, 0, prompt.c_str(), width - 1);
    wattroff(promptWindow, A_BOLD);
    if (colorsEnabled) wattroff(promptWindow, COLOR_PAIR(3));

    const int firstLineWidth = width - inputColumn - 1;
    if (firstLineWidth > 0) {
        mvwaddnstr(promptWindow, 1, inputColumn, inputBuffer.c_str(),
                   firstLineWidth);
    }

    int offset = std::max(0, firstLineWidth);
    int row = 2;
    while (offset < static_cast<int>(inputBuffer.size()) && row < promptRows) {
        mvwaddnstr(promptWindow, row, 0, inputBuffer.c_str() + offset,
                   width - 1);
        offset += width - 1;
        ++row;
    }
    const int cursorRow = std::min(row, promptRows - 1);
    const int cursorColumn = row == 1
        ? inputColumn + static_cast<int>(inputBuffer.size())
        : static_cast<int>(inputBuffer.size()) - offset + width - 1;
    wmove(promptWindow, cursorRow, std::min(cursorColumn, width - 1));
    refreshScreen();
}

void TerminalUI::refreshScreen() {
    wnoutrefresh(outputWindow);
    wnoutrefresh(promptWindow);
    doupdate();
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

void TerminalUI::clearOutput() {
    werase(outputWindow);
    refreshPrompt();
}

void TerminalUI::printOutput(const Presentation::Output& output) {
    if (output.color == Presentation::Color::Divider) {
        printDivider(output.text);
        return;
    }
    storeHistory(output.text);
    if (getcurx(outputWindow) != 0) waddch(outputWindow, '\n');
    int colorPair = 0;
    switch (output.color) {
    case Presentation::Color::Server: colorPair = 1; break;
    case Presentation::Color::Incoming: colorPair = 2; break;
    case Presentation::Color::Own: colorPair = 3; break;
    case Presentation::Color::OtherUser: colorPair = 4; break;
    case Presentation::Color::Error: colorPair = 5; break;
    case Presentation::Color::Divider: colorPair = 1; break;
    case Presentation::Color::Normal: break;
    }
        if (colorsEnabled && colorPair != 0) wattron(outputWindow, COLOR_PAIR(colorPair));
    if (output.dim) wattron(outputWindow, A_DIM);
    waddnstr(outputWindow, output.text.c_str(), -1);
    if (output.dim) wattroff(outputWindow, A_DIM);
        if (colorsEnabled && colorPair != 0) wattroff(outputWindow, COLOR_PAIR(colorPair));
    refreshPrompt();
}

void TerminalUI::printDivider(const std::string& text) {
    storeHistory(text);
    if (getcurx(outputWindow) != 0) waddch(outputWindow, '\n');

    const int height = getmaxy(outputWindow);
    int row = getcury(outputWindow);
        if (height < 3) {
            waddnstr(outputWindow, text.c_str(), -1);
            waddch(outputWindow, '\n');
            refreshPrompt();
            return;
        }
    if (row + 3 >= height) {
        wscrl(outputWindow, row + 4 - height);
        row = getcury(outputWindow);
    }
    const int lastColumn = getmaxx(outputWindow) - 1;

    wattron(outputWindow, A_DIM);
    mvwhline(outputWindow, row, 0, ACS_HLINE, getmaxx(outputWindow));
    wattroff(outputWindow, A_DIM);

    wattron(outputWindow, A_DIM);
    mvwaddnstr(outputWindow, row + 1, 0, text.c_str(), lastColumn);
    wattroff(outputWindow, A_DIM);

    wattron(outputWindow, A_DIM);
    mvwhline(outputWindow, row + 2, 0, ACS_HLINE, getmaxx(outputWindow));
    wattroff(outputWindow, A_DIM);
    wmove(outputWindow, row + 2, 0);
    waddch(outputWindow, '\n');
    refreshPrompt();
}

void TerminalUI::printOutput(const std::string& message) {
    printOutput({message, Presentation::Color::Normal});
}

void TerminalUI::storeHistory(const std::string& text) {
    outputHistory.push_back(text);
    if (outputHistory.back().empty() || outputHistory.back().back() != '\n') {
        outputHistory.back().push_back('\n');
    }
}
