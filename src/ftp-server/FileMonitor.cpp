#include "FileMonitor.h"
#include "FtpClient.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdexcept>

std::filesystem::file_time_type FileMonitor::getFileModificationTime(const std::string& filepath) {
    return std::filesystem::last_write_time(filepath);
}

FileMonitor::FileMonitor(const Config& config) : config(config) {
    // Get initial modification time
    try {
        lastModified = getFileModificationTime(config.local_file);
    }
    catch (const std::exception& e) {
        std::cout << ansiColor(31) << "Error getting initial file time: " << e.what() << ansiReset() << std::endl;
        throw;
    }
}

void FileMonitor::monitor() {
    std::cout << ansiColor(97) << "Monitoring file: " << config.local_file << ansiReset() << std::endl;
    std::cout << ansiColor(97) << "Upload destination: " << config.remote_file << ansiReset() << std::endl;
    std::cout << ansiColor(97) << "Check interval: " << config.check_interval << " seconds" << ansiReset() << std::endl;
    
    while (true) {
        try {
            // Sleep for the check interval
            std::this_thread::sleep_for(std::chrono::seconds(config.check_interval));
            
            // Check if file has been modified
            auto currentModTime = getFileModificationTime(config.local_file);
            
            if (currentModTime > lastModified) {
                std::cout << ansiColor(97) << "File changed, uploading..." << ansiReset() << std::endl;
                
                // Upload file
                FtpClient client(config.ftp_server, config.ftp_port, 
                                config.ftp_user, config.ftp_password);
                client.connect();
                
                // Advanced feature: Try to resume upload if file exists
                try {
                    size_t remoteSize = client.getFileSize(config.remote_file);
                    size_t localSize = std::filesystem::file_size(config.local_file);
                    
                    if (remoteSize > 0 && remoteSize < localSize) {
                        std::cout << ansiColor(97) << "Attempting to resume upload from position: " << ansiReset() << remoteSize << std::endl;
                        client.resumeUpload(config.local_file, config.remote_file, remoteSize);
                    } else {
                        client.uploadFile(config.local_file, config.remote_file);
                    }
                }
                catch (const std::exception&) {
                    // If SIZE command fails or any other error, do a normal upload
                    client.uploadFile(config.local_file, config.remote_file);
                }
                
                client.disconnect();
                
                // Update last modified time
                lastModified = currentModTime;
            }
        }
        catch (const std::exception& e) {
            std::cout << ansiColor(97) << "Error during monitoring: " << e.what() << ansiReset() << std::endl;
            std::cout << ansiColor(97) << "Continuing to monitor..." << ansiReset() << std::endl;
        }
    }
}
