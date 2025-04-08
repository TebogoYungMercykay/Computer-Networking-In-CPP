// Contact - structure to store LDAP entry information
#ifndef CONTACT_H
#define CONTACT_H

#include <string>
#include <map>
#include <vector>

struct Contact {
    std::string dn;
    std::string name;
    std::string phoneNumber;
    std::map<std::string, std::vector<std::string>> attributes;
};

#endif // CONTACT_H
