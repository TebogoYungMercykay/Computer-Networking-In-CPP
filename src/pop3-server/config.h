// Config - Structure to store SMTP and POP3 config information
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    // POP3 configuration
    std::string pop3_host;
    int pop3_port;
    bool pop3_use_ssl;
    std::string sender_email;
    std::string password;

    Config() {
        // Default POP3 settings
        pop3_port = 995;
        pop3_use_ssl = true;
        pop3_host = "pop.gmail.com";
        
        sender_email = "";
        password = "";
    }
};

#endif // CONFIG_H
