#include "Config.h"
#include <fstream>
#include <stdexcept>

std::string trim(const std::string& str) {
    auto start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    auto end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

Config loadConfig(const std::string& envPath) {
    Config config;
    std::ifstream envFile(envPath);
    
    if (!envFile.is_open()) {
        throw std::runtime_error("Failed to open .env file");
    }
    
    std::string line;
    while (std::getline(envFile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        auto pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));
            
            if (key == "FTP_SERVER") config.ftp_server = value;
            else if (key == "FTP_PORT") config.ftp_port = std::stoi(value);
            else if (key == "FTP_USER") config.ftp_user = value;
            else if (key == "FTP_PASSWORD") config.ftp_password = value;
            else if (key == "LOCAL_FILE") config.local_file = value;
            else if (key == "REMOTE_FILE") config.remote_file = value;
            else if (key == "CHECK_INTERVAL") config.check_interval = std::stoi(value);
        }
    }
    
    // Set defaults for missing values
    if (config.ftp_port == 0) config.ftp_port = 21;
    if (config.check_interval == 0) config.check_interval = 60;
    
    return config;
}
