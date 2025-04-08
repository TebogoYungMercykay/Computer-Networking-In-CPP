#include "LDAPClient.h"
#include "BEREncoder.h"
#include "BERParser.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

LDAPClient::LDAPClient() : sock(-1), messageID(1) {}

LDAPClient::~LDAPClient() {
    close();
}

bool LDAPClient::connect(const char* host, int port) {
    // Socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Could not create socket" << std::endl;
        return false;
    }
    
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    // Connection - using :: to specify global namespace
    if (::connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        std::cerr << "Connect failed" << std::endl;
        return false;
    }
    
    std::cout << "Connected to " << host << ":" << port << std::endl;
    return true;
}

std::vector<unsigned char> LDAPClient::createLDAPBindRequest(int messageID, const char* bindDN, const char* password) {
    std::vector<unsigned char> result;
    
    // Message ID
    std::vector<unsigned char> messageIDBytes = BEREncoder::encodeInteger(messageID);
    
    // Bind Request components
    std::vector<unsigned char> bindRequestComponents;
    
    // Version (INTEGER 3)
    bindRequestComponents.push_back(0x02);
    bindRequestComponents.push_back(0x01);
    bindRequestComponents.push_back(0x03);
    
    // Bind DN (OCTET STRING)
    std::vector<unsigned char> bindDNBytes = BEREncoder::encodeOctetString(bindDN);
    bindRequestComponents.insert(bindRequestComponents.end(), bindDNBytes.begin(), bindDNBytes.end());
    
    // Authentication (OCTET STRING)
    
    bindRequestComponents.push_back(0x80);
    bindRequestComponents.push_back(static_cast<unsigned char>(strlen(password)));
    for (size_t i = 0; i < strlen(password); i++) {
        bindRequestComponents.push_back(static_cast<unsigned char>(password[i]));
    }
    
    // Wrap bind request components
    std::vector<unsigned char> bindRequestSequence;
    
    // BIND REQUEST tag
    bindRequestSequence.push_back(LDAP_BIND_REQUEST);
    
    std::vector<unsigned char> lengthBytes = BEREncoder::encodeLength(bindRequestComponents.size());
    bindRequestSequence.insert(bindRequestSequence.end(), lengthBytes.begin(), lengthBytes.end());
    
    bindRequestSequence.insert(bindRequestSequence.end(), 
                              bindRequestComponents.begin(), 
                              bindRequestComponents.end());
    
    // Combine messageID and bindRequestSequence into final result
    result.insert(result.end(), messageIDBytes.begin(), messageIDBytes.end());
    result.insert(result.end(), bindRequestSequence.begin(), bindRequestSequence.end());
    
    // Wrap everything in a SEQUENCE
    return BEREncoder::encodeSequence(result);
}

std::vector<unsigned char> LDAPClient::createEqualityFilter(const std::string& attribute, const std::string& value) {
    std::vector<unsigned char> filterBytes;
    
    // Context specific tag [3] for equality match
    filterBytes.push_back(LDAP_FILTER_EQUALITY);
    
    // Construct filter sequence: attribute name + value
    std::vector<unsigned char> filterSeq;
    
    // Attribute name
    std::vector<unsigned char> attrBytes = BEREncoder::encodeOctetString(attribute.c_str());
    filterSeq.insert(filterSeq.end(), attrBytes.begin(), attrBytes.end());
    
    // Value
    std::vector<unsigned char> valueBytes = BEREncoder::encodeOctetString(value.c_str());
    filterSeq.insert(filterSeq.end(), valueBytes.begin(), valueBytes.end());
    
    // Add length to filter
    std::vector<unsigned char> lenBytes = BEREncoder::encodeLength(filterSeq.size());
    filterBytes.insert(filterBytes.end(), lenBytes.begin(), lenBytes.end());
    
    // Add filter sequence to filterBytes
    filterBytes.insert(filterBytes.end(), filterSeq.begin(), filterSeq.end());
    
    return filterBytes;
}

std::vector<unsigned char> LDAPClient::createSubstringFilter(const std::string& attribute, const std::string& value) {
    std::vector<unsigned char> filterBytes;
    
    // Context specific tag [4] for substring match
    filterBytes.push_back(LDAP_FILTER_SUBSTRING);
    
    // Construct filter sequence
    std::vector<unsigned char> filterSeq;
    
    // Attribute name
    std::vector<unsigned char> attrBytes = BEREncoder::encodeOctetString(attribute.c_str());
    filterSeq.insert(filterSeq.end(), attrBytes.begin(), attrBytes.end());
    
    // Substrings - filter with *value*
    std::vector<unsigned char> substringsSeq;
    
    // Context specific tag [1] for "any" substring
    substringsSeq.push_back(0x81);
    substringsSeq.push_back(static_cast<unsigned char>(value.length()));
    for (char c : value) {
        substringsSeq.push_back(static_cast<unsigned char>(c));
    }
    
    // Wrap substrings in a SEQUENCE
    std::vector<unsigned char> substringsWrapper = BEREncoder::encodeSequence(substringsSeq);
    filterSeq.insert(filterSeq.end(), substringsWrapper.begin(), substringsWrapper.end());
    
    // Add length to filter
    std::vector<unsigned char> lenBytes = BEREncoder::encodeLength(filterSeq.size());
    filterBytes.insert(filterBytes.end(), lenBytes.begin(), lenBytes.end());
    
    // Add filter sequence to filterBytes
    filterBytes.insert(filterBytes.end(), filterSeq.begin(), filterSeq.end());
    
    return filterBytes;
}

std::vector<unsigned char> LDAPClient::createORFilter(const std::vector<std::vector<unsigned char>>& filters) {
    std::vector<unsigned char> orFilterComponents;
    
    // Combine all filters
    for (const auto& filter : filters) {
        orFilterComponents.insert(orFilterComponents.end(), filter.begin(), filter.end());
    }
    
    std::vector<unsigned char> orFilter;

    // Context specific tag [1] for OR
    orFilter.push_back(LDAP_FILTER_OR);
    
    // Add length
    std::vector<unsigned char> lenBytes = BEREncoder::encodeLength(orFilterComponents.size());
    orFilter.insert(orFilter.end(), lenBytes.begin(), lenBytes.end());
    
    // Add components
    orFilter.insert(orFilter.end(), orFilterComponents.begin(), orFilterComponents.end());
    
    return orFilter;
}

std::vector<unsigned char> LDAPClient::createLDAPSearchRequest(
    int messageID, const char* baseDN, 
    const char* filter, 
    const std::vector<std::string>& attributes,
    int scope
) {
    std::vector<unsigned char> result;
    
    // Message ID
    std::vector<unsigned char> messageIDBytes = BEREncoder::encodeInteger(messageID);
    
    // Search Request components
    std::vector<unsigned char> searchRequestComponents;
    
    // Base DN (OCTET STRING)
    std::vector<unsigned char> baseDNBytes = BEREncoder::encodeOctetString(baseDN);
    searchRequestComponents.insert(searchRequestComponents.end(), baseDNBytes.begin(), baseDNBytes.end());
    
    // Scope (ENUMERATED)
    searchRequestComponents.push_back(0x0A);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(static_cast<unsigned char>(scope));
    
    // Deref Aliases (ENUMERATED 3 = derefAlways)
    searchRequestComponents.push_back(0x0A);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x03);
    
    // Size Limit (INTEGER 0 = no limit)
    searchRequestComponents.push_back(0x02);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x00);
    
    // Time Limit (INTEGER 0 = no limit)
    searchRequestComponents.push_back(0x02);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x00);
    
    // Types Only (BOOLEAN false)
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x00);
    
    // Filter
    if (std::string(filter) == "(objectClass=*)") {
        // Present filter for (objectClass=*)
        searchRequestComponents.push_back(0x87);
        
        // Length of "objectClass"
        searchRequestComponents.push_back(0x0B);
        for (char c : std::string("objectClass")) {
            searchRequestComponents.push_back(static_cast<unsigned char>(c));
        }
    } else {
        // For simple filter: (cn=<filter>)
        std::vector<unsigned char> filterBytes = createEqualityFilter("cn", filter);
        searchRequestComponents.insert(searchRequestComponents.end(), filterBytes.begin(), filterBytes.end());
    }
    
    // Attributes to return (SEQUENCE OF OCTET STRING)
    std::vector<unsigned char> attrsSequence;
    
    for (const auto& attr : attributes) {
        std::vector<unsigned char> attrBytes = BEREncoder::encodeOctetString(attr.c_str());
        attrsSequence.insert(attrsSequence.end(), attrBytes.begin(), attrBytes.end());
    }
    
    // Wrap attributes in a SEQUENCE
    std::vector<unsigned char> attrsWrapper = BEREncoder::encodeSequence(attrsSequence);
    searchRequestComponents.insert(searchRequestComponents.end(), attrsWrapper.begin(), attrsWrapper.end());
    
    // Wrap search request components in a SEQUENCE with SEARCH REQUEST tag (0x63)
    std::vector<unsigned char> searchRequestSequence;
    searchRequestSequence.push_back(LDAP_SEARCH_REQUEST);
    
    std::vector<unsigned char> reqLenBytes = BEREncoder::encodeLength(searchRequestComponents.size());
    searchRequestSequence.insert(searchRequestSequence.end(), reqLenBytes.begin(), reqLenBytes.end());
    
    searchRequestSequence.insert(searchRequestSequence.end(), 
                                searchRequestComponents.begin(), 
                                searchRequestComponents.end());
    
    // Combine messageID and searchRequestSequence into final result
    result.insert(result.end(), messageIDBytes.begin(), messageIDBytes.end());
    result.insert(result.end(), searchRequestSequence.begin(), searchRequestSequence.end());
    
    // Wrap everything in a SEQUENCE
    return BEREncoder::encodeSequence(result);
}

std::string LDAPClient::parseLDAPSearchResponse(const std::vector<unsigned char>& response) {
    std::string result;
    BERParser parser(response);
    
    // Skip the outer SEQUENCE
    parser.readTag();
    parser.readLength();
    
    // Skip the Message ID
    parser.readInteger();
    
    while (parser.getPosition() < parser.getSize()) {
        unsigned char tag = parser.readTag();
        size_t len = parser.readLength();
        
        // Check if this is a search result entry
        if (tag == LDAP_SEARCH_RESULT_ENTRY) {
            size_t entryEnd = parser.getPosition() + len;
            
            // Skip the object name
            parser.skipEntry();
            
            // Read attributes (SEQUENCE OF SEQUENCE)
            unsigned char attrsTag = parser.readTag();
            parser.readLength();
            
            while (parser.getPosition() < entryEnd) {
                // Each attribute is a SEQUENCE
                parser.readTag();
                parser.readLength();
                
                // Read attribute type
                std::string attrType = parser.readOctetString();
                
                // Read attribute values (SET OF OCTET STRING)
                parser.readTag();
                parser.readLength();
                
                // Read the first value
                std::string attrValue = parser.readOctetString();
                
                if (attrType == "telephoneNumber") {
                    result = attrValue;
                }
            }
        } else if (tag == LDAP_SEARCH_RESULT_DONE) {
            // Read the result code
            parser.readInteger();
            break;
        } else {
            parser.readValue(len);
        }
    }
    
    return result;
}

std::vector<Contact> LDAPClient::parseLDAPSearchResponseMultiple(const std::vector<unsigned char>& response) {
    std::vector<Contact> contacts;
    BERParser parser(response);
    
    // Skip the outer SEQUENCE
    parser.readTag();
    parser.readLength();
    
    // Skip the Message ID
    parser.readInteger();
    
    while (parser.getPosition() < parser.getSize()) {
        unsigned char tag = parser.readTag();
        size_t len = parser.readLength();
        
        // Check if this is a search result entry
        if (tag == LDAP_SEARCH_RESULT_ENTRY) {
            size_t entryEnd = parser.getPosition() + len;
            Contact contact;
            
            // Read the object name (DN)
            contact.dn = parser.readOctetString();
            
            // Read attributes (SEQUENCE OF SEQUENCE)
            unsigned char attrsTag = parser.readTag();
            parser.readLength();
            
            while (parser.getPosition() < entryEnd) {
                // Each attribute is a SEQUENCE
                parser.readTag();
                parser.readLength();
                
                // Read attribute type
                std::string attrType = parser.readOctetString();
                
                // Read attribute values (SET OF OCTET STRING)
                parser.readTag();
                size_t valuesLen = parser.readLength();
                size_t valuesEnd = parser.getPosition() + valuesLen;
                
                std::vector<std::string> values;
                while (parser.getPosition() < valuesEnd) {
                    std::string attrValue = parser.readOctetString();
                    values.push_back(attrValue);
                    
                    // Store common attributes in easy-to-access fields
                    if (attrType == "cn" && contact.name.empty()) {
                        contact.name = attrValue;
                    } else if (attrType == "telephoneNumber" && contact.phoneNumber.empty()) {
                        contact.phoneNumber = attrValue;
                    }
                }
                
                // Store all attributes in the map
                contact.attributes[attrType] = values;
            }
            
            contacts.push_back(contact);
        } else if (tag == LDAP_SEARCH_RESULT_DONE) {
            // Read the result code
            parser.readInteger();
            break;
        } else {
            parser.readValue(len);
        }
    }
    
    return contacts;
}

bool LDAPClient::bind(const char* bindDN, const char* password) {
    std::vector<unsigned char> bindRequest = createLDAPBindRequest(messageID++, bindDN, password);
    
    // Send the bind request
    if (send(sock, bindRequest.data(), bindRequest.size(), 0) < 0) {
        std::cerr << "Send failed" << std::endl;
        return false;
    }
    
    // Receive response
    std::vector<unsigned char> response(1024, 0);
    int recvSize = recv(sock, response.data(), response.size(), 0);
    
    if (recvSize < 0) {
        std::cerr << "Receive failed" << std::endl;
        return false;
    }
    
    response.resize(recvSize);
    
    if (response.size() > 0 && response[0] == 0x30) {
        std::cout << "Bind successful" << std::endl;
        return true;
    } else {
        std::cerr << "Bind failed" << std::endl;
        return false;
    }
}

std::string LDAPClient::search(const char* baseDN, const char* filter) {
    std::vector<std::string> attributes = {"telephoneNumber"};
    std::vector<unsigned char> searchRequest = createLDAPSearchRequest(messageID++, baseDN, filter, attributes);
    
    // Send the search request
    if (send(sock, searchRequest.data(), searchRequest.size(), 0) < 0) {
        std::cerr << "Send failed" << std::endl;
        return "";
    }
    
    std::vector<unsigned char> response(2048, 0);
    int recvSize = recv(sock, response.data(), response.size(), 0);
    
    if (recvSize < 0) {
        std::cerr << "Receive failed" << std::endl;
        return "";
    }
    
    response.resize(recvSize);
    
    // Parse the response to extract the telephone number
    return parseLDAPSearchResponse(response);
}

std::vector<Contact> LDAPClient::searchAll(const char* baseDN, const char* filter) {
    std::vector<std::string> attributes = {"cn", "telephoneNumber"};
    std::vector<unsigned char> searchRequest = createLDAPSearchRequest(messageID++, baseDN, filter, attributes);
    
    // Send the search request
    if (send(sock, searchRequest.data(), searchRequest.size(), 0) < 0) {
        std::cerr << "Send failed" << std::endl;
        return {};
    }
    
    std::vector<unsigned char> response(8192, 0);
    int recvSize = recv(sock, response.data(), response.size(), 0);
    
    if (recvSize < 0) {
        std::cerr << "Receive failed" << std::endl;
        return {};
    }
    
    response.resize(recvSize);
    
    // Parse the response to extract all contacts
    return parseLDAPSearchResponseMultiple(response);
}

std::vector<Contact> LDAPClient::advancedSearch(const char* baseDN, const std::string& nameFilter, const std::string& phoneFilter) {
    std::vector<std::vector<unsigned char>> filters;
    std::vector<std::string> attributes = {"cn", "telephoneNumber"};
    
    // Create filters based on provided criteria
    if (!nameFilter.empty()) {
        filters.push_back(createSubstringFilter("cn", nameFilter));
    }
    
    if (!phoneFilter.empty()) {
        filters.push_back(createSubstringFilter("telephoneNumber", phoneFilter));
    }
    
    // If no filters provided, return all contacts
    if (filters.empty()) {
        return searchAll(baseDN);
    }
    
    // Create OR filter if multiple conditions
    std::vector<unsigned char> finalFilter;
    if (filters.size() > 1) {
        finalFilter = createORFilter(filters);
    } else {
        finalFilter = filters[0];
    }
    
    // Create custom search request with the OR filter
    std::vector<unsigned char> result;
    
    // Message ID
    std::vector<unsigned char> messageIDBytes = BEREncoder::encodeInteger(messageID++);
    
    // Search Request components
    std::vector<unsigned char> searchRequestComponents;
    
    // Base DN (OCTET STRING)
    std::vector<unsigned char> baseDNBytes = BEREncoder::encodeOctetString(baseDN);
    searchRequestComponents.insert(searchRequestComponents.end(), baseDNBytes.begin(), baseDNBytes.end());
    
    // Scope (ENUMERATED 2 = subtree)
    searchRequestComponents.push_back(0x0A);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x02);
    
    // Deref Aliases (ENUMERATED 3 = derefAlways)
    searchRequestComponents.push_back(0x0A);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x03);
    
    // Size Limit (INTEGER 0 = no limit)
    searchRequestComponents.push_back(0x02);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x00);
    
    // Time Limit (INTEGER 0 = no limit)
    searchRequestComponents.push_back(0x02);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x00);
    
    // Types Only (BOOLEAN false)
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x01);
    searchRequestComponents.push_back(0x00);
    
    // Add the custom filter
    searchRequestComponents.insert(searchRequestComponents.end(), finalFilter.begin(), finalFilter.end());
    
    // Attributes to return (SEQUENCE OF OCTET STRING)
    std::vector<unsigned char> attrsSequence;
    
    for (const auto& attr : attributes) {
        std::vector<unsigned char> attrBytes = BEREncoder::encodeOctetString(attr.c_str());
        attrsSequence.insert(attrsSequence.end(), attrBytes.begin(), attrBytes.end());
    }
    
    // Wrap attributes in a SEQUENCE
    std::vector<unsigned char> attrsWrapper = BEREncoder::encodeSequence(attrsSequence);
    searchRequestComponents.insert(searchRequestComponents.end(), attrsWrapper.begin(), attrsWrapper.end());
    
    // Wrap search request components in a SEQUENCE with SEARCH REQUEST tag
    std::vector<unsigned char> searchRequestSequence;
    searchRequestSequence.push_back(LDAP_SEARCH_REQUEST);
    
    std::vector<unsigned char> reqLenBytes = BEREncoder::encodeLength(searchRequestComponents.size());
    searchRequestSequence.insert(searchRequestSequence.end(), reqLenBytes.begin(), reqLenBytes.end());
    
    searchRequestSequence.insert(searchRequestSequence.end(), 
                                searchRequestComponents.begin(), 
                                searchRequestComponents.end());
    
    // Combine messageID and searchRequestSequence into final result
    result.insert(result.end(), messageIDBytes.begin(), messageIDBytes.end());
    result.insert(result.end(), searchRequestSequence.begin(), searchRequestSequence.end());
    
    // Wrap everything in a SEQUENCE
    std::vector<unsigned char> searchRequest = BEREncoder::encodeSequence(result);
    
    // Send the request
    if (send(sock, searchRequest.data(), searchRequest.size(), 0) < 0) {
        std::cerr << "Send failed" << std::endl;
        return {};
    }
    
    // Receive response
    std::vector<unsigned char> response(8192, 0);
    int recvSize = recv(sock, response.data(), response.size(), 0);
    
    if (recvSize < 0) {
        std::cerr << "Receive failed" << std::endl;
        return {};
    }
    
    response.resize(recvSize);
    
    // Parse the response to extract all contacts
    return parseLDAPSearchResponseMultiple(response);
}

void LDAPClient::close() {
    if (sock != -1) {
        ::close(sock);
        sock = -1;
    }
}
