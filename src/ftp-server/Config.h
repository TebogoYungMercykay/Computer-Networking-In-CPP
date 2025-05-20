#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include "utils.h"

// Configuration structure
struct Config {
    std::string ftp_server;
    int ftp_port;
    std::string ftp_user;
    std::string ftp_password;
    std::string local_file;
    std::string remote_file;
    int check_interval; // seconds
};

// Load configuration from .env file
Config loadConfig(const std::string& envPath);

// Helper function to trim whitespace
std::string trim(const std::string& str);

#endif // CONFIG_H
