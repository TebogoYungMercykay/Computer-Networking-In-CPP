#ifndef FILE_MONITOR_H
#define FILE_MONITOR_H

#include "Config.h"
#include "FtpClient.h"
#include <filesystem>

// File monitoring class
class FileMonitor {
    private:
        Config config;
        std::filesystem::file_time_type lastModified;
        
        // Public methods
        
        std::filesystem::file_time_type getFileModificationTime(const std::string& filepath);
        bool uploadFile();
        void createDirectoryStructure(FtpClient& client, const std::string& remotePath);
        void monitorWithPolling();
    public:
        FileMonitor(const Config& config);
        void monitor();
};

#endif // FILE_MONITOR_H
