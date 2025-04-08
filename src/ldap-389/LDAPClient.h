// LDAPClient - Manages the LDAP connection, request creation, and response handling.

#ifndef LDAP_CLIENT_H
#define LDAP_CLIENT_H

#include <string>
#include <vector>
#include <netinet/in.h>

// LDAP Operation Codes
#define LDAP_BIND_REQUEST 0x60
#define LDAP_BIND_RESPONSE 0x61
#define LDAP_SEARCH_REQUEST 0x63
#define LDAP_SEARCH_RESULT_ENTRY 0x64
#define LDAP_SEARCH_RESULT_DONE 0x65

// LDAP Result Codes
#define LDAP_SUCCESS 0
#define LDAP_INVALID_CREDENTIALS 49

class LDAPClient {
    private:
        int sock;
        struct sockaddr_in server;
        int messageID;
        
        std::vector<unsigned char> createLDAPBindRequest(int messageID, const char* bindDN, const char* password);
        std::vector<unsigned char> createLDAPSearchRequest(int messageID, const char* baseDN, const char* filter);
        std::string parseLDAPSearchResponse(const std::vector<unsigned char>& response);
        
    public:
        LDAPClient();
        ~LDAPClient();
        
        bool connect(const char* host, int port);
        bool bind(const char* bindDN, const char* password);
        std::string search(const char* baseDN, const char* filter);
        void close();
};

#endif // LDAP_CLIENT_H
