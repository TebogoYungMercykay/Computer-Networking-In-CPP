#ifndef SMTP_CLIENT_H
#define SMTP_CLIENT_H

#include <string>
#include <openssl/ssl.h>

class SMTPClient {
private:
    int socket_fd;
    SSL* ssl;
    SSL_CTX* ctx;
    bool use_ssl;
    
    // Send data through socket or SSL
    bool send_data(const std::string& data);
    
    // Receive data from socket or SSL
    std::string receive_data();
    
    // Wait for response and check response code
    bool check_response(int expected_code);

public:
    SMTPClient(bool use_ssl = true);
    ~SMTPClient();
    
    bool connect_to_server(const std::string& host, int port);
    bool authenticate(const std::string& username, const std::string& password);
    bool send_email(const std::string& from, const std::string& to, 
                    const std::string& subject, const std::string& body);
    bool quit();
    void close_connection();
};

#endif // SMTP_CLIENT_H
