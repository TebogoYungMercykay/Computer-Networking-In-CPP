// ---- Utilities Class Implementation ----

#include "utils.h"
#include <iostream>
#include <fstream>
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

// Base64 encoding function
std::string base64_encode(const std::string& input) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string encoded;
    int i = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = input.size();
    int input_index = 0;
    
    while (in_len--) {
        // Fix: Use separate variables for indexing
        char_array_3[i] = input[input_index];
        i++;
        input_index++;
        
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(int j = 0; j < 4; j++)
                encoded += base64_chars[char_array_4[j]];
            i = 0;
        }
    }

    if (i) {
        for(int j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (int j = 0; j < i + 1; j++)
            encoded += base64_chars[char_array_4[j]];

        while(i++ < 3)
            encoded += '=';
    }

    return encoded;
}

// Read character without waiting for Enter key
char getch_nonblock() {
    char buf = 0;
    struct termios old;
    memset(&old, 0, sizeof(old));
    
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 0;
    old.c_cc[VTIME] = 0;
    
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    
    if (read(0, &buf, 1) < 0)
        perror("read()");
    
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror("tcsetattr ~ICANON");
    
    return buf;
}

// Get input with standard canonical mode
std::string getInput() {
    std::string input;
    std::getline(std::cin, input);
    return input;
}

// Load configuration from .env file
Config load_config() {
    Config config;
    std::ifstream file(".env");
    
    if (!file.is_open()) {
        std::cerr << ansiColor(31) << "Failed to open .env file. Using default configuration." << ansiReset() << std::endl;
        return config;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // Parse key=value pairs
        size_t delimiter_pos = line.find('=');
        if (delimiter_pos != std::string::npos) {
            std::string key = line.substr(0, delimiter_pos);
            std::string value = line.substr(delimiter_pos + 1);
            
            // Remove quotes if present
            if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                value = value.substr(1, value.size() - 2);
            }
            
            // Set configuration values
            if (key == "POP3_HOST") {
                config.pop3_host = value;
            } else if (key == "POP3_PORT") {
                config.pop3_port = std::stoi(value);
            } else if (key == "POP3_USE_SSL") {
                config.pop3_use_ssl = (value == "true" || value == "1");
            } else if (key == "EMAIL") {
                config.sender_email = value;
            } else if (key == "PASSWORD") {
                config.password = value;
            }
        }
    }
    
    return config;
}

