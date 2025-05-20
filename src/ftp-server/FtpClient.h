#ifndef FTP_CLIENT_H
#define FTP_CLIENT_H

#include <string>
#include <utility>
#include "Config.h"

// FTP Client class
class FtpClient {
private:
    int controlSocket;
    std::string server;
    int port;
    std::string username;
    std::string password;
    
    // Read response from FTP server
    std::string readResponse();
    
    // Send command to FTP server
    std::string sendCommand(const std::string& command);
    
    // Parse passive mode response
    std::pair<std::string, int> parsePassiveMode(const std::string& response);
    
    // Create data connection in passive mode
    int createDataConnection();
    
public:
    FtpClient(const std::string& server, int port, 
              const std::string& username, const std::string& password);
    
    ~FtpClient();
    
    // Connect to FTP server and login
    void connect();
    
    // Upload file to server
    void uploadFile(const std::string& localPath, const std::string& remotePath);
    
    // Disconnect from server
    void disconnect();
    
    // Advanced functionality - get file size
    size_t getFileSize(const std::string& remotePath);
    
    // Advanced functionality - resume upload
    void resumeUpload(const std::string& localPath, const std::string& remotePath, size_t offset);
    
    // Advanced functionality - list directory
    std::string listDirectory(const std::string& path = "");
};

#endif // FTP_CLIENT_H
