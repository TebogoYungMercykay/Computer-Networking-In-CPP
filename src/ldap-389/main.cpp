#include "LDAPClient.h"
#include <iostream>
#include <string>

int main() {
    LDAPClient client;
    
    // Connect to OpenLDAP server
    if (!client.connect("127.0.0.1", 389)) {
        return 1;
    }
    
    // Bind to LDAP server (anonymous bind in this case)
    if (!client.bind("", "")) {
        return 1;
    }
    
    std::string friendName;
    std::cout << "Enter friend name to lookup: ";
    std::getline(std::cin, friendName);
    
    // Search for the friend's telephone number
    std::string phoneNumber = client.search("ou=Friends,dc=friends,dc=local", friendName.c_str());
    
    if (!phoneNumber.empty()) {
        std::cout << "Telephone number for " << friendName << ": " << phoneNumber << std::endl;
    } else {
        std::cout << "Friend not found or no telephone number available." << std::endl;
    }
    
    client.close();
    return 0;
}
