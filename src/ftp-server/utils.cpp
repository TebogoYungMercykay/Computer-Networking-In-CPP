// ---- Utilities Class Implementation ----

#include "utils.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

std::string ansiColor(int color) {
    return "\033[" + std::to_string(color) + "m";
}

std::string ansiReset() {
    return "\033[0m";
}

std::string clearScreen() {
    return "\033[2J\033[H";
}

