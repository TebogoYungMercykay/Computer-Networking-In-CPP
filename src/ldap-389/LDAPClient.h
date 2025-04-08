#ifndef LDAP_CLIENT_H
#define LDAP_CLIENT_H

#include <string>
#include <vector>
#include <map>
#include <netinet/in.h>
#include "Contact.h"

// LDAP Operation Codes
#define LDAP_BIND_REQUEST 0x60
#define LDAP_BIND_RESPONSE 0x61
#define LDAP_SEARCH_REQUEST 0x63
#define LDAP_SEARCH_RESULT_ENTRY 0x64
#define LDAP_SEARCH_RESULT_DONE 0x65

// LDAP Result Codes
#define LDAP_SUCCESS 0
#define LDAP_INVALID_CREDENTIALS 49

// LDAP Search Scope
#define LDAP_SCOPE_BASE 0
#define LDAP_SCOPE_ONELEVEL 1
#define LDAP_SCOPE_SUBTREE 2

// LDAP Filter Types
#define LDAP_FILTER_EQUALITY 0xA3
#define LDAP_FILTER_SUBSTRING 0xA4
#define LDAP_FILTER_OR 0xA1

class LDAPClient {
    private:
        int sock;
        struct sockaddr_in server;
        int messageID;
        
        std::vector<unsigned char> createLDAPBindRequest(int messageID, const char* bindDN, const char* password);
        std::vector<unsigned char> createLDAPSearchRequest(int messageID, const char* baseDN, const char* filter, const std::vector<std::string>& attributes, int scope = LDAP_SCOPE_SUBTREE);
        std::vector<Contact> parseLDAPSearchResponseMultiple(const std::vector<unsigned char>& response);
        std::string parseLDAPSearchResponse(const std::vector<unsigned char>& response);
        std::vector<unsigned char> createEqualityFilter(const std::string& attribute, const std::string& value);
        std::vector<unsigned char> createSubstringFilter(const std::string& attribute, const std::string& value);
        std::vector<unsigned char> createORFilter(const std::vector<std::vector<unsigned char>>& filters);
        
    public:
        LDAPClient();
        ~LDAPClient();
        
        bool connect(const char* host, int port);
        bool bind(const char* bindDN, const char* password);
        std::string search(const char* baseDN, const char* filter);
        std::vector<Contact> searchAll(const char* baseDN, const char* filter = "(objectClass=*)");
        std::vector<Contact> advancedSearch(const char* baseDN, const std::string& nameFilter, const std::string& phoneFilter);
        void close();
};

#endif // LDAP_CLIENT_H
