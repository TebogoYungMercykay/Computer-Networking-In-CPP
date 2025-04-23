// EmailInfo - Structure to store POP3 Email information
#ifndef EMAILINFO_H
#define EMAILINFO_H

#include <string>

struct EmailInfo {
    int id;
    std::string from;
    std::string subject;
    std::string date;
    int size_bytes;
    bool marked_for_deletion;
    
    EmailInfo() : id(0), size_bytes(0), marked_for_deletion(false) {}
};

#endif // EMAILINFO_H
