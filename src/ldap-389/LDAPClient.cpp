#include "LDAPClient.h"
#include "BEREncoder.h"
#include "BERParser.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

LDAPClient::LDAPClient() : sock(-1), messageID(1) {}

LDAPClient::~LDAPClient() {
    close();
}

bool LDAPClient::connect(const char* host, int port) {
    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Could not create socket" << std::endl;
        return false;
    }
    
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    
    // Connect to server - use :: to specify global namespace
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
    
    // Authentication (OCTET STRING with context tag [0])
    bindRequestComponents.push_back(0x80);  // Context tag [0] for Simple Authentication
    bindRequestComponents.push_back(static_cast<unsigned char>(strlen(password)));
    for (size_t i = 0; i < strlen(password); i++) {
        bindRequestComponents.push_back(static_cast<unsigned char>(password[i]));
    }
    
    // Wrap bind request components in a SEQUENCE with BIND REQUEST tag (0x60)
    std::vector<unsigned char> bindRequestSequence;
    bindRequestSequence.push_back(LDAP_BIND_REQUEST);  // BIND REQUEST tag
    
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

std::vector<unsigned char> LDAPClient::createLDAPSearchRequest(int messageID, const char* baseDN, const char* filter) {
    std::vector<unsigned char> result;
    
    // Message ID
    std::vector<unsigned char> messageIDBytes = BEREncoder::encodeInteger(messageID);
    
    // Search Request components
    std::vector<unsigned char> searchRequestComponents;
    
    // Base DN (OCTET STRING)
    std::vector<unsigned char> baseDNBytes = BEREncoder::encodeOctetString(baseDN);
    searchRequestComponents.insert(searchRequestComponents.end(), baseDNBytes.begin(), baseDNBytes.end());
    
    // Scope (ENUMERATED 2 = subtree)
    searchRequestComponents.push_back(0x0A);  // ENUMERATED
    searchRequestComponents.push_back(0x01);  // Length 1
    searchRequestComponents.push_back(0x02);  // Value 2 (subtree)
    
    // Deref Aliases (ENUMERATED 3 = derefAlways)
    searchRequestComponents.push_back(0x0A);  // ENUMERATED
    searchRequestComponents.push_back(0x01);  // Length 1
    searchRequestComponents.push_back(0x03);  // Value 3 (derefAlways)
    
    // Size Limit (INTEGER 0 = no limit)
    searchRequestComponents.push_back(0x02);  // INTEGER
    searchRequestComponents.push_back(0x01);  // Length 1
    searchRequestComponents.push_back(0x00);  // Value 0 (no limit)
    
    // Time Limit (INTEGER 0 = no limit)
    searchRequestComponents.push_back(0x02);  // INTEGER
    searchRequestComponents.push_back(0x01);  // Length 1
    searchRequestComponents.push_back(0x00);  // Value 0 (no limit)
    
    // Types Only (BOOLEAN false)
    searchRequestComponents.push_back(0x01);  // BOOLEAN
    searchRequestComponents.push_back(0x01);  // Length 1
    searchRequestComponents.push_back(0x00);  // Value false
    
    // Filter (custom encoding for simplicity - we'll use a simple equality filter)
    // Format: (cn=<filter>) which is an equality match on the cn attribute
    std::vector<unsigned char> filterBytes;
    filterBytes.push_back(0xA3);  // Context specific tag [3] for equality match
    
    // Construct filter sequence: attribute name + value
    std::vector<unsigned char> filterSeq;
    
    // Attribute: "cn"
    std::vector<unsigned char> attrBytes = BEREncoder::encodeOctetString("cn");
    filterSeq.insert(filterSeq.end(), attrBytes.begin(), attrBytes.end());
    
    // Value: the search string
    std::vector<unsigned char> valueBytes = BEREncoder::encodeOctetString(filter);
    filterSeq.insert(filterSeq.end(), valueBytes.begin(), valueBytes.end());
    
    // Add length to filter
    std::vector<unsigned char> lenBytes = BEREncoder::encodeLength(filterSeq.size());
    filterBytes.insert(filterBytes.end(), lenBytes.begin(), lenBytes.end());
    
    // Add filter sequence to filterBytes
    filterBytes.insert(filterBytes.end(), filterSeq.begin(), filterSeq.end());
    
    // Add filter to search request
    searchRequestComponents.insert(searchRequestComponents.end(), filterBytes.begin(), filterBytes.end());
    
    // Attributes to return (SEQUENCE OF OCTET STRING)
    // For now, we'll request just "telephoneNumber"
    std::vector<unsigned char> attrsSequence;
    std::vector<unsigned char> telNoBytes = BEREncoder::encodeOctetString("telephoneNumber");
    attrsSequence.insert(attrsSequence.end(), telNoBytes.begin(), telNoBytes.end());
    
    // Wrap attributes in a SEQUENCE
    std::vector<unsigned char> attrsWrapper = BEREncoder::encodeSequence(attrsSequence);
    searchRequestComponents.insert(searchRequestComponents.end(), attrsWrapper.begin(), attrsWrapper.end());
    
    // Wrap search request components in a SEQUENCE with SEARCH REQUEST tag (0x63)
    std::vector<unsigned char> searchRequestSequence;
    searchRequestSequence.push_back(LDAP_SEARCH_REQUEST);  // SEARCH REQUEST tag
    
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
    parser.readTag();  // Should be 0x30 (SEQUENCE)
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
            unsigned char attrsTag = parser.readTag();  // Should be 0x30 (SEQUENCE)
            parser.readLength();
            
            while (parser.getPosition() < entryEnd) {
                // Each attribute is a SEQUENCE
                parser.readTag();  // Should be 0x30 (SEQUENCE)
                parser.readLength();
                
                // Read attribute type
                std::string attrType = parser.readOctetString();
                
                // Read attribute values (SET OF OCTET STRING)
                parser.readTag();  // Should be 0x31 (SET)
                parser.readLength();
                
                // Read the first value
                std::string attrValue = parser.readOctetString();
                
                if (attrType == "telephoneNumber") {
                    result = attrValue;
                }
            }
        } else if (tag == LDAP_SEARCH_RESULT_DONE) {
            // Read the result code
            parser.readInteger();  // Should be LDAP_SUCCESS (0) if successful
            break;
        } else {
            // Skip other entries
            parser.readValue(len);
        }
    }
    
    return result;
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
    
    // Parse response (simplified)
    // In a real implementation, we would parse the response properly
    if (response.size() > 0 && response[0] == 0x30) {  // SEQUENCE
        std::cout << "Bind successful" << std::endl;
        return true;
    } else {
        std::cerr << "Bind failed" << std::endl;
        return false;
    }
}

std::string LDAPClient::search(const char* baseDN, const char* filter) {
    std::vector<unsigned char> searchRequest = createLDAPSearchRequest(messageID++, baseDN, filter);
    
    // Send the search request
    if (send(sock, searchRequest.data(), searchRequest.size(), 0) < 0) {
        std::cerr << "Send failed" << std::endl;
        return "";
    }
    
    // Receive response (this is simplified, in reality we might need to read multiple packets)
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

void LDAPClient::close() {
    if (sock != -1) {
        ::close(sock);
        sock = -1;
    }
}
