#include "FtpClient.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <regex>
#include <stdexcept>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <cstring>
#include <netdb.h>

FtpClient::FtpClient(const std::string& server, int port, 
                     const std::string& username, const std::string& password)
    : controlSocket(-1), server(server), port(port), username(username), password(password) {
}

FtpClient::~FtpClient() {
    if (controlSocket >= 0) {
        close(controlSocket);
    }
}

std::string FtpClient::readResponse() {
    std::vector<char> buffer(4096);
    std::string response;
    
    while (true) {
        int received = recv(controlSocket, buffer.data(), buffer.size() - 1, 0);
        if (received <= 0) {
            throw std::runtime_error("Connection closed by server");
        }
        buffer[received] = '\0';
        response += buffer.data();
        
        if (received < static_cast<int>(buffer.size() - 1)) {
            size_t pos = response.rfind("\r\n");
            if (pos != std::string::npos && pos >= 3 && response[pos-4] == ' ') {
                break;
            }
            if (response.size() >= 4 && response[3] == ' ') {
                break;
            }
        }
    }
    
    std::cout << ansiColor(90) << "Server: " << response << ansiReset() << std::endl;
    return response;
}

std::string FtpClient::sendCommand(const std::string& command) {
    std::string displayCommand = command;

    std::regex passwordRegex(R"((PASS\s+)(.*))", std::regex_constants::icase);
    std::regex userRegex(R"((USER\s+)(.*))", std::regex_constants::icase);

    if (std::regex_search(command, passwordRegex)) {
        displayCommand = std::regex_replace(command, passwordRegex, "$1************");
    } else if (std::regex_search(command, userRegex)) {
        displayCommand = std::regex_replace(command, userRegex, "$1************");
    }

    std::cout << ansiColor(90) << "Client: " << displayCommand << ansiReset() << std::endl;
    std::string cmd = command + "\r\n";
    if (send(controlSocket, cmd.c_str(), cmd.length(), 0) == -1) {
        throw std::runtime_error("Failed to send command");
    }
    return readResponse();
}

std::pair<std::string, int> FtpClient::parsePassiveMode(const std::string& response) {
    std::regex pasvRegex(R"(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))");
    std::smatch matches;
    
    if (std::regex_search(response, matches, pasvRegex) && matches.size() == 7) {
        std::string ip = matches[1].str() + "." + matches[2].str() + "." + 
                         matches[3].str() + "." + matches[4].str();
        int port = std::stoi(matches[5].str()) * 256 + std::stoi(matches[6].str());
        return {ip, port};
    }
    
    throw std::runtime_error("Failed to parse passive mode response");
}

int FtpClient::createDataConnection() {
    std::string response = sendCommand("PASV");
    auto [ip, port] = parsePassiveMode(response);
    
    std::cout << ansiColor(94) << "Debug: Connecting to data socket at " << ip << ":" << port << ansiReset() << std::endl;
    
    int dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (dataSocket == -1) {
        throw std::runtime_error("Failed to create data socket");
    }
    
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (ip == "127,0,0,1") {
        ip = "127.0.0.1";
    }
    
    if (ip.substr(0, 3) == "172" || ip.substr(0, 3) == "10." || ip.substr(0, 7) == "192.168") {
        std::cout << ansiColor(94) << "Debug: FTP server returned internal IP " << ip 
                  << ", using server address from config instead." << ansiReset() << std::endl;
        ip = server;
    }
    
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        close(dataSocket);
        std::string error = "Invalid address format: " + ip;
        throw std::runtime_error(error);
    }
    
    if (::connect(dataSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        close(dataSocket);
        std::string error = "Failed to connect to data port: " + std::string(strerror(errno));
        throw std::runtime_error(error);
    }
    
    return dataSocket;
}

void FtpClient::connect() {
    // Create control socket
    controlSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (controlSocket == -1) {
        throw std::runtime_error("Failed to create control socket");
    }
    
    int optval = 1;
    if (setsockopt(controlSocket, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0) {
        std::cerr << "Warning: Failed to set SO_KEEPALIVE" << std::endl;
    }
    
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    std::string portStr = std::to_string(port);
    int status = getaddrinfo(server.c_str(), portStr.c_str(), &hints, &res);
    if (status != 0) {
        close(controlSocket);
        std::string error = "Failed to resolve hostname: ";
        error += gai_strerror(status);
        throw std::runtime_error(error);
    }
    
    bool connected = false;
    for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
        if (::connect(controlSocket, p->ai_addr, p->ai_addrlen) == 0) {
            connected = true;
            break;
        }
    }
    
    freeaddrinfo(res);
    
    if (!connected) {
        close(controlSocket);
        throw std::runtime_error("Failed to connect to server: " + std::string(strerror(errno)));
    }
    
    try {
        // Read welcome message
        readResponse();
        
        // Login
        sendCommand("USER " + username);
        sendCommand("PASS " + password);
    }
    catch (const std::exception& e) {
        close(controlSocket);
        throw std::runtime_error(std::string("Connection failed: ") + e.what());
    }
}

void FtpClient::uploadFile(const std::string& localPath, const std::string& remotePath) {
    try {
        // Switch to binary mode
        sendCommand("TYPE I");
        
        // Create data connection
        int dataSocket = createDataConnection();
        sendCommand("STOR " + remotePath);
        
        std::ifstream file(localPath, std::ios::binary);
        if (!file.is_open()) {
            close(dataSocket);
            throw std::runtime_error("Failed to open local file");
        }
        
        std::vector<char> buffer(8192);
        while (!file.eof()) {
            file.read(buffer.data(), buffer.size());
            std::streamsize bytesRead = file.gcount();
            
            if (bytesRead > 0) {
                if (send(dataSocket, buffer.data(), bytesRead, 0) == -1) {
                    close(dataSocket);
                    throw std::runtime_error("Failed to send file data");
                }
            }
        }
        
        close(dataSocket);
        readResponse();
        
        std::cout << ansiColor(92) << "File uploaded successfully: " << localPath << " -> " << remotePath << ansiReset() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << ansiColor(91) << "Error during file upload: " << e.what() << ansiReset() << std::endl;
        throw;
    }
}

void FtpClient::disconnect() {
    if (controlSocket >= 0) {
        try {
            sendCommand("QUIT");
        } catch (...) {
            // Ignore errors during disconnect
        }
        close(controlSocket);
        controlSocket = -1;
    }
}

size_t FtpClient::getFileSize(const std::string& remotePath) {
    std::string response = sendCommand("SIZE " + remotePath);
    std::regex sizeRegex(R"(213 (\d+))");
    std::smatch matches;
    
    if (std::regex_search(response, matches, sizeRegex) && matches.size() == 2) {
        return std::stoul(matches[1].str());
    }
    
    throw std::runtime_error("Failed to get file size");
}

void FtpClient::resumeUpload(const std::string& localPath, const std::string& remotePath, size_t offset) {
    try {
        sendCommand("TYPE I");
        sendCommand("REST " + std::to_string(offset));
        
        // Create data connection
        int dataSocket = createDataConnection();
        sendCommand("STOR " + remotePath);
        
        std::ifstream file(localPath, std::ios::binary);
        if (!file.is_open()) {
            close(dataSocket);
            throw std::runtime_error("Failed to open local file");
        }
        
        file.seekg(offset);
        
        std::vector<char> buffer(8192);
        while (!file.eof()) {
            file.read(buffer.data(), buffer.size());
            std::streamsize bytesRead = file.gcount();
            
            if (bytesRead > 0) {
                if (send(dataSocket, buffer.data(), bytesRead, 0) == -1) {
                    close(dataSocket);
                    throw std::runtime_error("Failed to send file data");
                }
            }
        }
        
        // Close data connection
        close(dataSocket);
        readResponse();
        
        std::cout << ansiColor(92) << "File resumed upload successfully: " << localPath << " -> " << remotePath << ansiReset() << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << ansiColor(91) << "Error during file resume upload: " << e.what() << ansiReset() << std::endl;
        throw;
    }
}

std::string FtpClient::listDirectory(const std::string& path) {
    // Create data connection
    int dataSocket = createDataConnection();
    sendCommand("LIST " + path);
    
    std::vector<char> buffer(4096);
    std::string listing;
    
    while (true) {
        int received = recv(dataSocket, buffer.data(), buffer.size() - 1, 0);
        if (received <= 0) break;
        
        buffer[received] = '\0';
        listing += buffer.data();
    }
    
    close(dataSocket);
    readResponse();
    
    return listing;
}
