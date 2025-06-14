// ---- SMTP Client Class Implementation ----

#include "smtp_client.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <ctime>


// Create A Functio to Generate a Random String
std::string generateRandomString(size_t length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string random_string;
    
    for (size_t i = 0; i < length; ++i) {
        random_string += characters[rand() % characters.size()];
    }
    
    return random_string;
}

// ---- Private methods ----

bool SMTPClient::send_data(const std::string& data) {
    if (use_ssl && ssl) {
        return SSL_write(ssl, data.c_str(), data.length()) > 0;
    } else {
        return send(socket_fd, data.c_str(), data.length(), 0) > 0;
    }
}

std::string SMTPClient::receive_data() {
    char buffer[1024] = {0};
    int bytes_read;
    
    if (use_ssl && ssl) {
        bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    } else {
        bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    }
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        return std::string(buffer);
    }
    
    return "";
}

bool SMTPClient::check_response(int expected_code) {
    std::string response = receive_data();
    std::cout << ansiColor(90) << "Server: " << response << ansiReset();
    
    try {
        int response_code = std::stoi(response.substr(0, 3));
        return response_code == expected_code;
    } catch (...) {
        return false;
    }
}

// ---- Public methods ----

SMTPClient::SMTPClient(bool param_ssl) {
    socket_fd = -1;
    ssl = nullptr;
    ctx = nullptr;
    use_ssl = param_ssl;
}

SMTPClient::~SMTPClient() {
    close_connection();
}

bool SMTPClient::connect_to_server(const std::string& host, int port) {
    struct hostent *server = gethostbyname(host.c_str());
    if (server == nullptr) {
        std::cerr << ansiColor(31) << "Error: Could not resolve hostname " << host << ansiReset() << std::endl;
        return false;
    }
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << ansiColor(31) << "Error: Could not create socket" << ansiReset() << std::endl;
        return false;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << ansiColor(31) << "Error: Connection failed" << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // Initial response from server (non-SSL)
    char buffer[1024] = {0};
    int bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        std::cerr << ansiColor(31) << "Error: Failed to receive initial server response" << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    buffer[bytes_read] = '\0';
    std::cout << ansiColor(90) << "Server: " << buffer << ansiReset();
    
    // Check if the response starts with "220"
    if (strncmp(buffer, "220", 3) != 0) {
        std::cerr << ansiColor(31) << "Error: Server did not respond with ready status" << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // Send EHLO (non-SSL initially)
    std::string ehlo_cmd = "EHLO " + host + "\r\n";
    send(socket_fd, ehlo_cmd.c_str(), ehlo_cmd.length(), 0);
    
    // Read response to EHLO (non-SSL)
    bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        std::cerr << ansiColor(31) << "Error: Failed to receive EHLO response" << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    buffer[bytes_read] = '\0';
    std::cout << ansiColor(90) << "Server: " << buffer << ansiReset();
    
    // Check if the response starts with "250"
    if (strncmp(buffer, "250", 3) != 0) {
        std::cerr << ansiColor(31) << "Error: EHLO command failed" << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // For Gmail we always need to use STARTTLS
    if (host.find("gmail") != std::string::npos || use_ssl) {
        // Send STARTTLS command (non-SSL)
        std::string starttls_cmd = "STARTTLS\r\n";
        send(socket_fd, starttls_cmd.c_str(), starttls_cmd.length(), 0);
        
        // Read response to STARTTLS (non-SSL)
        bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) {
            std::cerr << ansiColor(31) << "Error: Failed to receive STARTTLS response" << ansiReset() << std::endl;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        buffer[bytes_read] = '\0';
        std::cout << ansiColor(90) << "Server: " << buffer << ansiReset();
        
        // Check if the response starts with "220"
        if (strncmp(buffer, "220", 3) != 0) {
            std::cerr << ansiColor(31) << "Error: STARTTLS command failed" << ansiReset() << std::endl;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        // Initialize SSL
        if (ctx == nullptr) {
            SSL_library_init();
            OpenSSL_add_all_algorithms();
            SSL_load_error_strings();
            ctx = SSL_CTX_new(TLS_client_method());
            
            if (!ctx) {
                ERR_print_errors_fp(stderr);
                std::cerr << ansiColor(31) << "Error: Failed to create SSL context" << ansiReset() << std::endl;
                close(socket_fd);
                socket_fd = -1;
                return false;
            }
        }
        
        // Create new SSL connection
        ssl = SSL_new(ctx);
        if (!ssl) {
            std::cerr << ansiColor(31) << "Error: Failed to create SSL structure" << ansiReset() << std::endl;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        SSL_set_fd(ssl, socket_fd);
        
        // Connect with SSL
        if (SSL_connect(ssl) != 1) {
            std::cerr << ansiColor(31) << "Error: SSL connection failed" << ansiReset() << std::endl;
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            ssl = nullptr;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        // EHLO again after TLS established
        if (!send_data(ehlo_cmd)) {
            std::cerr << ansiColor(31) << "Error: Failed to send EHLO after TLS" << ansiReset() << std::endl;
            return false;
        }
        
        if (!check_response(250)) {
            std::cerr << ansiColor(31) << "Error: EHLO command failed after TLS" << ansiReset() << std::endl;
            return false;
        }
        
        // Set use_ssl to true since we're now using SSL
        use_ssl = true;
    }
    
    return true;
}

// * Secure Implementation
bool SMTPClient::authenticate(const std::string& username, const std::string& password) {
    // AUTH LOGIN command
    send_data("AUTH LOGIN\r\n");
    if (!check_response(334)) {
        std::cerr << ansiColor(31) << "Error: AUTH command failed" << ansiReset() << std::endl;
        return false;
    }
    
    // Base64 encoded Username
    send_data(base64_encode(username) + "\r\n");
    if (!check_response(334)) {
        std::cerr << ansiColor(31) << "Error: Username authentication failed" << ansiReset() << std::endl;
        return false;
    }
    
    // Base64 encoded Password
    send_data(base64_encode(password) + "\r\n");
    if (!check_response(235)) {
        std::cerr << ansiColor(31) << "Error: Password authentication failed" << ansiReset() << std::endl;
        return false;
    }
    
    return true;
}

bool SMTPClient::send_email(const std::string& from, const std::string& to, 
                    const std::string& subject, const std::string& body) {
    // MAIL FROM command
    std::string mail_from = "MAIL FROM:<" + from + ">\r\n";
    send_data(mail_from);
    if (!check_response(250)) {
        std::cerr << ansiColor(31) << "Error: MAIL FROM command failed" << ansiReset() << std::endl;
        return false;
    }
    
    // RCPT TO command
    std::string rcpt_to = "RCPT TO:<" + to + ">\r\n";
    send_data(rcpt_to);
    if (!check_response(250)) {
        std::cerr << ansiColor(31) << "Error: RCPT TO command failed" << ansiReset() << std::endl;
        return false;
    }
    
    // DATA command
    send_data("DATA\r\n");
    if (!check_response(354)) {
        std::cerr << ansiColor(31) << "Error: DATA command failed" << ansiReset() << std::endl;
        return false;
    }
    
    // Email Headers and Body
    time_t now = time(0);
    char date_buf[100];
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S %z", localtime(&now));
    std::string randomString = generateRandomString(6);
    std::string email_content = 
        "From: \"Home Security System\" <" + from + ">\r\n"
        "To: <" + to + ">\r\n"
        "Subject: " + randomString + " " + subject + "\r\n"
        "Date: " + std::string(date_buf) + "\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n" +
        body + randomString + "\r\n.\r\n";
    
    send_data(email_content);
    if (!check_response(250)) {
        std::cerr << ansiColor(31) << "Error: Email content failed to send" << ansiReset() << std::endl;
        return false;
    }
    
    return true;
}

bool SMTPClient::quit() {
    send_data("QUIT\r\n");
    return check_response(221);
}

void SMTPClient::close_connection() {
    if (ssl != nullptr) {
        SSL_free(ssl);
        ssl = nullptr;
    }
    
    if (ctx != nullptr) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
    
    if (socket_fd != -1) {
        close(socket_fd);
        socket_fd = -1;
    }
}
