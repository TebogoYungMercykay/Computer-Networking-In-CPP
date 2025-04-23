// ---- POP3 Client Class Implementation ----

#include "pop3_client.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <regex>

// ---- Private methods ----

bool POP3Client::send_command(const std::string& command) {
    std::string cmd_with_crlf = command + "\r\n";
    
    std::cout << ansiColor(90) << "Client: " << command << ansiReset() << std::endl;
    
    if (use_ssl && ssl) {
        return SSL_write(ssl, cmd_with_crlf.c_str(), cmd_with_crlf.length()) > 0;
    } else {
        return send(socket_fd, cmd_with_crlf.c_str(), cmd_with_crlf.length(), 0) > 0;
    }
}

std::string POP3Client::receive_response() {
    char buffer[4096] = {0};
    int bytes_read;
    std::string complete_response;
    
    if (use_ssl && ssl) {
        bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    } else {
        bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
    }
    
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        complete_response = std::string(buffer);
        
        // For multiline responses in POP3
        if (bytes_read == sizeof(buffer) - 1) {
            while (true) {
                if (use_ssl && ssl) {
                    bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
                } else {
                    bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
                }
                
                if (bytes_read <= 0) break;
                
                buffer[bytes_read] = '\0';
                complete_response += std::string(buffer);
                
                // Check if we've reached the end of the response (line with just a period)
                if (complete_response.find("\r\n.\r\n") != std::string::npos) {
                    break;
                }
            }
        }
    }
    
    std::cout << ansiColor(90) << "Server: " << complete_response << ansiReset();
    return complete_response;
}

bool POP3Client::check_response(const std::string& expected_prefix) {
    std::string response = receive_response();
    return response.compare(0, expected_prefix.length(), expected_prefix) == 0;
}

std::string POP3Client::extract_message_header(const std::string& header_data, const std::string& header_name) {
    std::regex header_regex(header_name + ":\\s*([^\r\n]+)", std::regex_constants::icase);
    std::smatch matches;
    
    if (std::regex_search(header_data, matches, header_regex) && matches.size() > 1) {
        return matches[1].str();
    }
    
    return "";
}

// ---- Public methods ----

POP3Client::POP3Client(bool param_ssl) {
    socket_fd = -1;
    ssl = nullptr;
    ctx = nullptr;
    use_ssl = param_ssl;
}

POP3Client::~POP3Client() {
    close_connection();
}

bool POP3Client::connect_to_server(const std::string& host, int port) {
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
    
    // Initial greeting from server (non-SSL)
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
    
    // Check if the response starts with "+OK"
    if (strncmp(buffer, "+OK", 3) != 0) {
        std::cerr << ansiColor(31) << "Error: Server did not respond with ready status" << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // For SSL connections (POP3S)
    if (use_ssl) {
        // Initialize SSL
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        
        // For explicit TLS (STLS command)
        if (host.find("gmail") != std::string::npos) {
            // Send STLS command
            std::string stls_cmd = "STLS";
            send(socket_fd, (stls_cmd + "\r\n").c_str(), stls_cmd.length() + 2, 0);
            
            // Read response
            bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                std::cerr << ansiColor(31) << "Error: Failed to receive STLS response" << ansiReset() << std::endl;
                close(socket_fd);
                socket_fd = -1;
                return false;
            }
            buffer[bytes_read] = '\0';
            std::cout << ansiColor(90) << "Server: " << buffer << ansiReset();
            
            // Check if response starts with "+OK"
            if (strncmp(buffer, "+OK", 3) != 0) {
                std::cerr << ansiColor(31) << "Error: STLS command failed" << ansiReset() << std::endl;
                close(socket_fd);
                socket_fd = -1;
                return false;
            }
        }
        
        // Create SSL context
        ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            ERR_print_errors_fp(stderr);
            std::cerr << ansiColor(31) << "Error: Failed to create SSL context" << ansiReset() << std::endl;
            close(socket_fd);
            socket_fd = -1;
            return false;
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
    }
    
    return true;
}

bool POP3Client::authenticate(const std::string& username, const std::string& password) {
    // Send USER command
    if (!send_command("USER " + username)) {
        std::cerr << ansiColor(31) << "Error: Failed to send USER command" << ansiReset() << std::endl;
        return false;
    }
    
    if (!check_response("+OK")) {
        std::cerr << ansiColor(31) << "Error: USER command failed" << ansiReset() << std::endl;
        return false;
    }
    
    // Send PASS command
    if (!send_command("PASS " + password)) {
        std::cerr << ansiColor(31) << "Error: Failed to send PASS command" << ansiReset() << std::endl;
        return false;
    }
    
    if (!check_response("+OK")) {
        std::cerr << ansiColor(31) << "Error: Authentication failed" << ansiReset() << std::endl;
        return false;
    }
    
    return true;
}

bool POP3Client::get_mailbox_status(int& message_count, int& mailbox_size) {
    message_count = 0;
    mailbox_size = 0;
    
    if (!send_command("STAT")) {
        std::cerr << ansiColor(31) << "Error: Failed to send STAT command" << ansiReset() << std::endl;
        return false;
    }
    
    std::string response = receive_response();
    if (response.compare(0, 3, "+OK") != 0) {
        std::cerr << ansiColor(31) << "Error: STAT command failed" << ansiReset() << std::endl;
        return false;
    }
    
    // Parse response: +OK {message_count} {mailbox_size}
    std::istringstream iss(response.substr(4)); // Skip "+OK "
    iss >> message_count >> mailbox_size;
    
    return true;
}

std::vector<EmailInfo> POP3Client::list_messages() {
    std::vector<EmailInfo> email_list;
    
    // First, get message count with STAT
    int message_count, mailbox_size;
    if (!get_mailbox_status(message_count, mailbox_size)) {
        return email_list; // Return empty list on failure
    }
    
    // Get message sizes with LIST
    if (!send_command("LIST")) {
        std::cerr << ansiColor(31) << "Error: Failed to send LIST command" << ansiReset() << std::endl;
        return email_list;
    }
    
    std::string response = receive_response();
    if (response.compare(0, 3, "+OK") != 0) {
        std::cerr << ansiColor(31) << "Error: LIST command failed" << ansiReset() << std::endl;
        return email_list;
    }
    
    // Parse the LIST response to get message sizes
    std::map<int, int> message_sizes;
    std::istringstream iss(response);
    std::string line;
    
    // Skip the first line with +OK
    std::getline(iss, line);
    
    // Parse each message line
    while (std::getline(iss, line)) {
        // Remove \r if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Stop at terminating "."
        if (line == ".") {
            break;
        }
        
        std::istringstream line_stream(line);
        int msg_id, msg_size;
        if (line_stream >> msg_id >> msg_size) {
            message_sizes[msg_id] = msg_size;
        }
    }
    
    // For each message, get headers using TOP command
    for (const auto& msg_pair : message_sizes) {
        int msg_id = msg_pair.first;
        int msg_size = msg_pair.second;
        
        if (!send_command("TOP " + std::to_string(msg_id) + " 0")) {
            std::cerr << ansiColor(31) << "Error: Failed to send TOP command for message " << msg_id << ansiReset() << std::endl;
            continue;
        }
        
        std::string headers = receive_response();
        if (headers.compare(0, 3, "+OK") != 0) {
            std::cerr << ansiColor(31) << "Error: TOP command failed for message " << msg_id << ansiReset() << std::endl;
            continue;
        }
        
        // Extract email info
        EmailInfo email;
        email.id = msg_id;
        email.size_bytes = msg_size;
        email.from = extract_message_header(headers, "From");
        email.subject = extract_message_header(headers, "Subject");
        email.marked_for_deletion = false;
        
        email_list.push_back(email);
    }
    
    return email_list;
}

bool POP3Client::delete_message(int message_id) {
    if (!send_command("DELE " + std::to_string(message_id))) {
        std::cerr << ansiColor(31) << "Error: Failed to send DELE command" << ansiReset() << std::endl;
        return false;
    }
    
    return check_response("+OK");
}

bool POP3Client::quit() {
    if (!send_command("QUIT")) {
        std::cerr << ansiColor(31) << "Error: Failed to send QUIT command" << ansiReset() << std::endl;
        return false;
    }
    
    return check_response("+OK");
}

void POP3Client::close_connection() {
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
