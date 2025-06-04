#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <termios.h>
#include "Config.h"

std::string ansiColor(int color);
std::string ansiReset();
std::string clearScreen();

#endif // UTILS_H
