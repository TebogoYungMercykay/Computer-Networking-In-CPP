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

// Private methods
bool SMTPClient::send_data(const std::string& data) {
    if (use_ssl) {
        return SSL_write(ssl, data.c_str(), data.length()) > 0;
    } else {
        return send(socket_fd, data.c_str(), data.length(), 0) > 0;
    }
}

std::string SMTPClient::receive_data() {
    char buffer[1024] = {0};
    int bytes_read;
    
    if (use_ssl) {
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

// Public methods
SMTPClient::SMTPClient(bool use_ssl) : socket_fd(-1), ssl(nullptr), ctx(nullptr), use_ssl(use_ssl) {
    if (use_ssl) {
        // Initialize OpenSSL
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ctx = SSL_CTX_new(TLS_client_method());
        
        if (!ctx) {
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
    }
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
    
    // Check initial response from server
    if (!check_response(220)) {
        std::cerr << ansiColor(31) << "Error: Server did not respond with ready status" << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // For SSL/TLS connections
    if (use_ssl) {
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, socket_fd);
        
        // Send EHLO before starting TLS
        std::string ehlo_cmd = "EHLO " + host + "\r\n";
        send_data(ehlo_cmd);
        if (!check_response(250)) {
            std::cerr << ansiColor(31) << "Error: EHLO command failed" << ansiReset() << std::endl;
            return false;
        }
        
        // Start TLS
        send_data("STARTTLS\r\n");
        if (!check_response(220)) {
            std::cerr << ansiColor(31) << "Error: STARTTLS command failed" << ansiReset() << std::endl;
            return false;
        }
        
        if (SSL_connect(ssl) != 1) {
            std::cerr << ansiColor(31) << "Error: SSL connection failed" << ansiReset() << std::endl;
            ERR_print_errors_fp(stderr);
            return false;
        }
        
        // Send EHLO again after TLS established
        send_data(ehlo_cmd);
        if (!check_response(250)) {
            std::cerr << ansiColor(31) << "Error: EHLO command failed after TLS" << ansiReset() << std::endl;
            return false;
        }
    } else {
        // Send HELO for non-SSL connections
        std::string helo_cmd = "HELO " + host + "\r\n";
        send_data(helo_cmd);
        return check_response(250);
    }
    
    return true;
}

bool SMTPClient::authenticate(const std::string& username, const std::string& password) {
    // AUTH LOGIN command
    send_data("AUTH LOGIN\r\n");
    if (!check_response(334)) {
        std::cerr << ansiColor(31) << "Error: AUTH command failed" << ansiReset() << std::endl;
        return false;
    }
    
    // Send base64 encoded username
    send_data(base64_encode(username) + "\r\n");
    if (!check_response(334)) {
        std::cerr << ansiColor(31) << "Error: Username authentication failed" << ansiReset() << std::endl;
        return false;
    }
    
    // Send base64 encoded password
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
    
    // Email headers and body
    time_t now = time(0);
    char date_buf[100];
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S %z", localtime(&now));
    
    std::string email_content = 
        "From: \"Home Security System\" <" + from + ">\r\n"
        "To: <" + to + ">\r\n"
        "Subject: " + subject + "\r\n"
        "Date: " + std::string(date_buf) + "\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "\r\n" +
        body + "\r\n.\r\n";
    
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
