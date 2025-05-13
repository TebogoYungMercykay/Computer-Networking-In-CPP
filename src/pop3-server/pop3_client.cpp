// pop3_client.cpp
#include "pop3_client.h"
#include "utils.h"
#include <iostream>
#include <string>
#include <cstring>
#include <chrono>
#include <thread>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <regex>
#include <fcntl.h>
#include <errno.h>

// ---- Private methods ----

bool POP3Client::send_command(const std::string& command) {
    std::string cmd_with_crlf = command + "\r\n";
    
    // TODO: Uncomment The Following Line of Code
    std::cout << ansiColor(90) << "Client: " << command << ansiReset() << std::endl;
    
    int result = 0;
    if (use_ssl && ssl) {
        result = SSL_write(ssl, cmd_with_crlf.c_str(), cmd_with_crlf.length());
    } else {
        result = send(socket_fd, cmd_with_crlf.c_str(), cmd_with_crlf.length(), 0);
    }
    
    if (result <= 0) {
        if (use_ssl) {
            ERR_print_errors_fp(stderr);
        }
        return false;
    }
    return true;
}

std::string POP3Client::receive_response(int timeout_seconds) {
    char buffer[4096] = {0};
    int bytes_read;
    std::string complete_response;
    
    // Set socket to non-blocking mode
    if (!use_ssl) {
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    }
    
    // Wait for data with timeout
    fd_set read_fds;
    struct timeval tv;
    int select_result;
    
    time_t start_time = time(NULL);
    
    while (true) {
        // Check if we've timed out
        if (time(NULL) - start_time > timeout_seconds) {
            // std::cerr << ansiColor(31) << "Error: Receive timeout" << ansiReset() << std::endl;
            break;
        }
        
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        
        tv.tv_sec = 1; 
        tv.tv_usec = 0;
        
        select_result = select(socket_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (select_result == -1) {
            // std::cerr << ansiColor(31) << "Error: Select failed: " << strerror(errno) << ansiReset() << std::endl;
            break;
        } else if (select_result == 0) {
            continue;
        }
        
        // Data is available to read
        if (use_ssl && ssl) {
            bytes_read = SSL_read(ssl, buffer, sizeof(buffer) - 1);
            
            // Handle SSL errors
            if (bytes_read <= 0) {
                int ssl_error = SSL_get_error(ssl, bytes_read);
                if (ssl_error == SSL_ERROR_WANT_READ || ssl_error == SSL_ERROR_WANT_WRITE) {
                    continue;
                } else {
                    ERR_print_errors_fp(stderr);
                    break;
                }
            }
        } else {
            bytes_read = recv(socket_fd, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                } else {
                    // std::cerr << ansiColor(31) << "Error: Receive failed: " << strerror(errno) << ansiReset() << std::endl;
                    break;
                }
            } else if (bytes_read == 0) {
                break;
            }
        }
        
        buffer[bytes_read] = '\0';
        complete_response += std::string(buffer);
        
        // Check if we've received a complete response
        if (!use_ssl || 
            (complete_response.find("\r\n.\r\n") != std::string::npos) || 
            (complete_response.find("\r\n") != std::string::npos && 
             complete_response.find("+OK") == 0)) {
            break;
        }
    }
    
    // Restore socket to blocking mode
    if (!use_ssl) {
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK);
    }
    
    // TODO: Uncomment The Following Line of Code
    std::cout << ansiColor(90) << "Server: " << complete_response << ansiReset();
    return complete_response;
}

bool POP3Client::check_response(const std::string& expected_prefix) {
    std::string response = receive_response();
    return response.compare(0, expected_prefix.length(), expected_prefix) == 0;
}

std::string POP3Client::extract_message_header(const std::string& header_data, const std::string& header_name) {
    std::regex header_regex("(^|\r\n|\n)(" + header_name + "):\\s*([^\r\n]+)((\r\n\\s+[^\r\n]+)*)", 
                          std::regex_constants::icase);
    std::smatch matches;
    
    if (std::regex_search(header_data, matches, header_regex) && matches.size() > 3) {
        std::string result = matches[3].str();
        
        if (matches[4].length() > 0) {
            std::string continuations = matches[4].str();
            std::regex fold_regex("\\r?\\n\\s+");
            result += std::regex_replace(continuations, fold_regex, " ");
        }
        
        auto start = result.find_first_not_of(" \t");
        if (start != std::string::npos) {
            result = result.substr(start);
        }
        
        auto end = result.find_last_not_of(" \t\r\n");
        if (end != std::string::npos) {
            result = result.substr(0, end + 1);
        }
        
        return result;
    }
    
    return "";
}

// ---- Public methods ----

POP3Client::POP3Client(bool param_ssl) {
    socket_fd = -1;
    ssl = nullptr;
    ctx = nullptr;
    use_ssl = param_ssl;
    
    // Initialize OpenSSL once
    static bool ssl_initialized = false;
    if (!ssl_initialized) {
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ssl_initialized = true;
    }
}

POP3Client::~POP3Client() {
    close_connection();
}

bool POP3Client::connect_to_server(const std::string& host, int port) {
    // Close any existing connection
    close_connection();
    
    // Resolve hostname
    struct hostent *server = gethostbyname(host.c_str());
    if (server == nullptr) {
        std::cerr << ansiColor(31) << "Error: Could not resolve hostname " << host << ansiReset() << std::endl;
        return false;
    }
    
    // Create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        std::cerr << ansiColor(31) << "Error: Could not create socket" << ansiReset() << std::endl;
        return false;
    }
    
    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        std::cerr << ansiColor(31) << "Warning: Could not set socket receive timeout" << ansiReset() << std::endl;
    }
    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        std::cerr << ansiColor(31) << "Warning: Could not set socket send timeout" << ansiReset() << std::endl;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    
    // Connect to server
    std::cout << "Connecting to " << host << " on port " << port << "..." << std::endl;
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << ansiColor(31) << "Error: Connection failed: " << strerror(errno) << ansiReset() << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    // For SSL connections
    if (use_ssl) {
        std::cout << "Initializing SSL connection..." << std::endl;
        
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
            SSL_CTX_free(ctx);
            ctx = nullptr;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        SSL_set_fd(ssl, socket_fd);
        
        // Connect with SSL
        int ssl_connect_result = SSL_connect(ssl);
        if (ssl_connect_result != 1) {
            int ssl_error = SSL_get_error(ssl, ssl_connect_result);
            std::cerr << ansiColor(31) << "Error: SSL connection failed with error code: " << ssl_error << ansiReset() << std::endl;
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            ssl = nullptr;
            SSL_CTX_free(ctx);
            ctx = nullptr;
            close(socket_fd);
            socket_fd = -1;
            return false;
        }
        
        // Get the initial server greeting over SSL
        std::string greeting = receive_response(10);
        if (greeting.empty() || greeting.compare(0, 3, "+OK") != 0) {
            std::cerr << ansiColor(31) << "Error: Invalid server greeting or no response" << ansiReset() << std::endl;
            close_connection();
            return false;
        }
    } else {
        // For non-SSL connections, get the server greeting
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
    std::istringstream iss(response.substr(4));
    iss >> message_count >> mailbox_size;
    
    return true;
}

// Updated function with delay and confidentiality filtering
std::vector<EmailInfo> POP3Client::list_messages() {
    std::vector<EmailInfo> email_list;
    
    int message_count, mailbox_size;
    if (!get_mailbox_status(message_count, mailbox_size)) {
        return email_list;
    }
    
    if (!send_command("LIST")) {
        std::cerr << ansiColor(31) << "Error: Failed to send LIST command" << ansiReset() << std::endl;
        return email_list;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    std::string response = receive_response();
    if (response.compare(0, 3, "+OK") != 0) {
        std::cerr << ansiColor(31) << "Error: LIST command failed" << ansiReset() << std::endl;
        return email_list;
    }
    
    std::map<int, int> message_sizes;
    std::istringstream iss(response);
    std::string line;
    
    std::getline(iss, line);
    
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line == ".") {
            break;
        }
        
        std::istringstream line_stream(line);
        int msg_id, msg_size;
        if (line_stream >> msg_id >> msg_size) {
            message_sizes[msg_id] = msg_size;
        }
    }
    
    const std::vector<std::string> confidential_keywords = {
        "confidential", "verification", "private", "sensitive", "classified", "restricted",
        "internal only", "not for distribution", "secret", "proprietary", "linkedin", "facebook", "twitter",
        "confidentiality", "sensitivity", "x-sensitivity", "x-confidentiality", "security-classification",
        "x-classification", "x-privacy", "x-privacy-level", "x-privacy-classification"
    };
    
    for (const auto& msg_pair : message_sizes) {
        int msg_id = msg_pair.first;
        int msg_size = msg_pair.second;

        if (!send_command("TOP " + std::to_string(msg_id) + " 20")) {
            std::cerr << ansiColor(31) << "Error: Failed to send TOP command for message " << msg_id << ansiReset() << std::endl;
            continue;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::string headers = receive_response();
        if (headers.find("Content-Type") == std::string::npos) {
            if (!send_command("RETR " + std::to_string(msg_id))) {
                std::cerr << ansiColor(31) << "Error: Failed to send RETR command for message " << msg_id << ansiReset() << std::endl;
                continue;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            
            headers = receive_response();
            if (headers.find("Content-Type") == std::string::npos) {
                std::cerr << ansiColor(31) << "Error: RETR command failed for message " << msg_id << ansiReset() << std::endl;
                continue;
            }
            
            size_t header_end = headers.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                headers = headers.substr(0, header_end);
            }
        }

        EmailInfo email;
        email.id = msg_id;
        email.size_bytes = msg_size;
        
        email.from = extract_message_header(headers, "From");
        if (email.from.empty()) {
            email.from = extract_message_header(headers, "Sender");
            if (email.from.empty()) {
                email.from = extract_message_header(headers, "Return-Path");
                if (email.from.empty()) {
                    email.from = "(Confidential Sender)";
                }
            }
        }
        
        email.subject = extract_message_header(headers, "Subject");
        if (email.subject.empty()) {
            email.subject = "(Confidential Subject)";
        }
        
        email.date = extract_message_header(headers, "Date");
        if (email.date.empty()) {
            email.date = extract_message_header(headers, "Delivery-Date");
            if (email.date.empty()) {
                email.date = extract_message_header(headers, "Received");
                if (!email.date.empty()) {
                    size_t date_pos = email.date.find(";");
                    if (date_pos != std::string::npos && date_pos + 1 < email.date.length()) {
                        email.date = email.date.substr(date_pos + 1);
                        auto start = email.date.find_first_not_of(" \t");
                        if (start != std::string::npos) {
                            email.date = email.date.substr(start);
                        }
                    }
                }
            }
        }
        
        bool is_confidential = false;
        std::string lowercase_subject = email.subject;
        std::string lowercase_headers = headers;
        
        std::transform(lowercase_subject.begin(), lowercase_subject.end(), 
                      lowercase_subject.begin(), ::tolower);
        std::transform(lowercase_headers.begin(), lowercase_headers.end(), 
                      lowercase_headers.begin(), ::tolower);
        
        for (const auto& keyword : confidential_keywords) {
            if (lowercase_subject.find(keyword) != std::string::npos ||
                lowercase_headers.find("sensitivity: " + keyword) != std::string::npos ||
                lowercase_headers.find("x-sensitivity: " + keyword) != std::string::npos ||
                lowercase_headers.find("x-confidentiality: " + keyword) != std::string::npos) {
                is_confidential = true;
                break;
            }
        }
        
        if (lowercase_headers.find("x-classification") != std::string::npos ||
            lowercase_headers.find("security-classification") != std::string::npos) {
            is_confidential = true;
        }
        
        email.is_confidential = is_confidential;
        
        email.marked_for_deletion = false;
        email_list.push_back(email);
    }
    
    std::cout << ansiColor(33) << "Note: Confidential emails are flagged for secure handling" << ansiReset() << std::endl;
    
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
    
    bool result = check_response("+OK");
    close_connection();
    return result;
}

void POP3Client::close_connection() {
    if (ssl != nullptr) {
        SSL_shutdown(ssl);
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

std::string POP3Client::retrieve_message(int message_id) {
    if (!send_command("RETR " + std::to_string(message_id))) {
        std::cerr << ansiColor(31) << "Error: Failed to send RETR command" << ansiReset() << std::endl;
        return "";
    }
    
    std::string response = receive_response(30);
    if (response.compare(0, 3, "+OK") != 0) {
        std::cerr << ansiColor(31) << "Error: RETR command failed" << ansiReset() << std::endl;
        return "";
    }
    
    return response;
}
