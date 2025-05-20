#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H

#include "Config.h"
#include <filesystem>

// File monitoring class
class FileMonitor {
private:
    Config config;
    std::filesystem::file_time_type lastModified;
    
    // Get last modification time of a file
    std::filesystem::file_time_type getFileModificationTime(const std::string& filepath);
    
public:
    FileMonitor(const Config& config);
    
    // Monitor file for changes
    void monitor();
};

#endif // FILE_MONITOR_H
