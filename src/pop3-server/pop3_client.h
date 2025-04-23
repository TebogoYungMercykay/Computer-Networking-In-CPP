// pop3_client.h
#ifndef POP3_CLIENT_H
#define POP3_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <openssl/ssl.h>
#include "info.h"

class POP3Client {
    private:
        int socket_fd;
        SSL* ssl;
        SSL_CTX* ctx;
        bool use_ssl;
        
        bool send_command(const std::string& command);
        std::string receive_response(int timeout_seconds = 10);
        bool check_response(const std::string& expected_prefix);
        std::string extract_message_header(const std::string& header_data, const std::string& header_name);
        
    public:
        POP3Client(bool use_ssl = true);
        ~POP3Client();
        
        bool connect_to_server(const std::string& host, int port);
        bool authenticate(const std::string& username, const std::string& password);
        bool get_mailbox_status(int& message_count, int& mailbox_size);
        std::vector<EmailInfo> list_messages();
        std::string retrieve_message(int message_id);
        bool delete_message(int message_id);
        bool quit();
        void close_connection();
};

#endif // POP3_CLIENT_H
