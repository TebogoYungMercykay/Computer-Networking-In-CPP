#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <termios.h>
#include "config.h"

std::string ansiColor(int color);
std::string ansiReset();
std::string clearScreen();

std::string base64_encode(const std::string& input);

char getch_nonblock();

std::string getInput();

Config load_config();

#endif // UTILS_H
