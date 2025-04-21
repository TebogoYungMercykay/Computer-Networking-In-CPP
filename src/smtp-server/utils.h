#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <termios.h>

// ANSI color codes and terminal control sequences
std::string ansiColor(int color);
std::string ansiReset();
std::string clearScreen();

// Base64 encoding function
std::string base64_encode(const std::string& input);

// Read character without waiting for Enter key
char getch_nonblock();

// Get input with standard canonical mode
std::string getInput();

#endif // UTILS_H
