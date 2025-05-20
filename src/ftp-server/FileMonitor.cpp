#include "FileMonitor.h"
#include "FtpClient.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdexcept>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <filesystem>


FileMonitor::FileMonitor(const Config& config, std::function<void()> handler) : config(config), handler(handler) {
    try {
        lastModified = getFileModificationTime(config.local_file);
    }
    catch (const std::exception& e) {
        std::cout << ansiColor(31) << "Error getting initial file time: " << e.what() << ansiReset() << std::endl;
        throw;
    }
}

std::filesystem::file_time_type FileMonitor::getFileModificationTime(const std::string& filepath) {
    try {
        return std::filesystem::last_write_time(filepath);
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error getting file modification time: " << e.what() << std::endl;
        return std::filesystem::file_time_type::min();
    }
}

bool FileMonitor::uploadFile() {
    try {
        // Upload file
        FtpClient client(config.ftp_server, config.ftp_port, 
                        config.ftp_user, config.ftp_password);
        
        client.connect();
        
        try {
            size_t remoteSize = client.getFileSize(config.remote_file);
            size_t localSize = std::filesystem::file_size(config.local_file);
            
            if (remoteSize > 0 && remoteSize < localSize) {
                std::cout << ansiColor(97) << "Attempting to resume upload from position: " 
                          << remoteSize << ansiReset() << std::endl;
                client.resumeUpload(config.local_file, config.remote_file, remoteSize);
            } else {
                client.uploadFile(config.local_file, config.remote_file);
            }
        } catch (const std::exception& e) {
            client.uploadFile(config.local_file, config.remote_file);
        }
        
        // Call the handler function after successful upload
        if (handler) {
            handler();
        }
        
        client.disconnect();
        return true;
    } catch (const std::exception& e) {
        std::cout << ansiColor(91) << "Error during file upload: " << e.what() 
                  << ansiReset() << std::endl;
        return false;
    }
}

void FileMonitor::monitor() {
    std::cout << ansiColor(97) << "Monitoring file: " << config.local_file << ansiReset() << std::endl;
    std::cout << ansiColor(97) << "Upload destination: " << config.remote_file << ansiReset() << std::endl;
    
    int inotifyFd = inotify_init();

    if (inotifyFd == -1) {
        std::cerr << "Error initializing inotify" << std::endl;
        std::cout << ansiColor(97) << "Falling back to polling with check interval: " 
                  << config.check_interval << " seconds" << ansiReset() << std::endl;
        monitorWithPolling();
        return;
    }
    
    int wd = inotify_add_watch(inotifyFd, config.local_file.c_str(), 
                               IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO);
    
    if (wd == -1) {
        std::cerr << "Error adding watch for " << config.local_file << std::endl;
        close(inotifyFd);
        std::cout << ansiColor(97) << "Falling back to polling with check interval: " 
                  << config.check_interval << " seconds" << ansiReset() << std::endl;
        monitorWithPolling();
        return;
    }
    
    std::cout << ansiColor(97) << "Using real-time file monitoring (inotify)" << ansiReset() << std::endl;
    
    int flags = fcntl(inotifyFd, F_GETFL, 0);
    fcntl(inotifyFd, F_SETFL, flags | O_NONBLOCK);
    char buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
    
    while (true) {
        try {
            // Poll inotify events
            int length = read(inotifyFd, buffer, sizeof(buffer));
            
            if (length > 0) {
                const struct inotify_event *event;
                for (char *ptr = buffer; ptr < buffer + length; 
                     ptr += sizeof(struct inotify_event) + event->len) {
                    
                    event = reinterpret_cast<const struct inotify_event *>(ptr);
                    
                    if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO)) {
                        std::cout << ansiColor(92) << "File changed, uploading..." << ansiReset() << std::endl;
                        
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        
                        uploadFile();
                        lastModified = getFileModificationTime(config.local_file);
                    }
                }
            }
            
            if (config.check_interval > 0) {
                static auto lastCheck = std::chrono::steady_clock::now();
                auto now = std::chrono::steady_clock::now();
                
                if (now - lastCheck > std::chrono::seconds(config.check_interval)) {
                    auto currentModTime = getFileModificationTime(config.local_file);
                    
                    if (currentModTime > lastModified) {
                        std::cout << ansiColor(97) << "Periodic check: File changed, uploading..." 
                                  << ansiReset() << std::endl;
                        
                        uploadFile();
                        lastModified = currentModTime;
                    }
                    
                    lastCheck = now;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        catch (const std::exception& e) {
            std::cout << ansiColor(91) << "Error during monitoring: " << e.what() 
                      << ansiReset() << std::endl;
            std::cout << ansiColor(97) << "Continuing to monitor..." << ansiReset() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    inotify_rm_watch(inotifyFd, wd);
    close(inotifyFd);
}

void FileMonitor::monitorWithPolling() {
    while (true) {
        try {
            std::this_thread::sleep_for(std::chrono::seconds(config.check_interval));
            auto currentModTime = getFileModificationTime(config.local_file);
            
            if (currentModTime > lastModified) {
                std::cout << ansiColor(92) << "File changed, uploading..." << ansiReset() << std::endl;
                
                uploadFile();
                lastModified = currentModTime;
            }
        }
        catch (const std::exception& e) {
            std::cout << ansiColor(91) << "Error during monitoring: " << e.what() 
                      << ansiReset() << std::endl;
            std::cout << ansiColor(97) << "Continuing to monitor..." << ansiReset() << std::endl;
        }
    }
}
