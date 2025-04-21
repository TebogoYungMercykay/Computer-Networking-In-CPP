// Config - Structure to store SMTP config information
#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    std::string host;
    int port;
    std::string sender_email;
    std::string password;
    std::string receiver_email;
    bool use_ssl;

    Config() {
        port = 587;
        use_ssl = true;
        host = "smtp.gmail.com";
        
        sender_email = "";
        password = "";
        receiver_email = "";
    }
};

#endif // CONFIG_H
