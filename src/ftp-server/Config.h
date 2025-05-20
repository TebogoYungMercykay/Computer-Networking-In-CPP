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
    int check_interval;
};

Config loadConfig(const std::string& envPath);
std::string trim(const std::string& str);

#endif // CONFIG_H
