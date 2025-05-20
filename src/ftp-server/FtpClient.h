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
        
        // Public methods

        std::string readResponse();
        std::pair<std::string, int> parsePassiveMode(const std::string& response);
        int createDataConnection();
        
    public:
        FtpClient(const std::string& server, int port, 
            const std::string& username, const std::string& password);
        std::string sendCommand(const std::string& command);
        ~FtpClient();
        void connect();
        void uploadFile(const std::string& localPath, const std::string& remotePath);
        void disconnect();
        size_t getFileSize(const std::string& remotePath);
        void resumeUpload(const std::string& localPath, const std::string& remotePath, size_t offset);
        std::string listDirectory(const std::string& path = "");
};

#endif // FTP_CLIENT_H
