#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// LDAP Operation Codes
#define LDAP_BIND_REQUEST 0x60
#define LDAP_BIND_RESPONSE 0x61
#define LDAP_SEARCH_REQUEST 0x63
#define LDAP_SEARCH_RESULT_ENTRY 0x64
#define LDAP_SEARCH_RESULT_DONE 0x65

// LDAP Result Codes
#define LDAP_SUCCESS 0
#define LDAP_INVALID_CREDENTIALS 49

// Berval for octet strings
struct Berval {
    int length;
    char* data;

    Berval(const char* str) {
        length = strlen(str);
        data = new char[length + 1];
        strncpy(data, str, length);
        data[length] = '\0';
    }

    ~Berval() {
        delete[] data;
    }
};

// ASN.1 BER Encoding Helper Functions
std::vector<unsigned char> encodeLength(size_t len) {
    std::vector<unsigned char> result;
    
    if (len < 128) {
        result.push_back(static_cast<unsigned char>(len));
    } else {
        // Determine how many bytes we need
        size_t tmpLen = len;
        int numBytes = 0;
        
        while (tmpLen > 0) {
            tmpLen >>= 8;
            numBytes++;
        }
        
        result.push_back(static_cast<unsigned char>(0x80 | numBytes));
        
        // Add length bytes in big-endian order
        for (int i = numBytes - 1; i >= 0; i--) {
            result.push_back(static_cast<unsigned char>((len >> (i * 8)) & 0xFF));
        }
    }
    
    return result;
}

std::vector<unsigned char> encodeInteger(int value) {
    std::vector<unsigned char> result;
    
    // Type: INTEGER (0x02)
    result.push_back(0x02);
    
    // For simplicity, we'll handle only small positive integers
    result.push_back(0x01);  // Length: 1 byte
    result.push_back(static_cast<unsigned char>(value & 0xFF));
    
    return result;
}

std::vector<unsigned char> encodeOctetString(const char* str) {
    std::vector<unsigned char> result;
    size_t len = strlen(str);
    
    // Type: OCTET STRING (0x04)
    result.push_back(0x04);
    
    // Length
    std::vector<unsigned char> lengthBytes = encodeLength(len);
    result.insert(result.end(), lengthBytes.begin(), lengthBytes.end());
    
    // Value
    for (size_t i = 0; i < len; i++) {
        result.push_back(static_cast<unsigned char>(str[i]));
    }
    
    return result;
}

std::vector<unsigned char> encodeSequence(const std::vector<unsigned char>& data) {
    std::vector<unsigned char> result;
    
    // Type: SEQUENCE (0x30)
    result.push_back(0x30);
    
    // Length
    std::vector<unsigned char> lengthBytes = encodeLength(data.size());
    result.insert(result.end(), lengthBytes.begin(), lengthBytes.end());
    
    // Value (the data itself)
    result.insert(result.end(), data.begin(), data.end());
    
    return result;
}

std::vector<unsigned char> createLDAPBindRequest(int messageID, const char* bindDN, const char* password) {
    std::vector<unsigned char> result;
    
    // Message ID
    std::vector<unsigned char> messageIDBytes = encodeInteger(messageID);
    
    // Bind Request components
    std::vector<unsigned char> bindRequestComponents;
    
    // Version (INTEGER 3)
    bindRequestComponents.push_back(0x02);
    bindRequestComponents.push_back(0x01);
    bindRequestComponents.push_back(0x03);
    
    // Bind DN (OCTET STRING)
    std::vector<unsigned char> bindDNBytes = encodeOctetString(bindDN);
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
    
    std::vector<unsigned char> lengthBytes = encodeLength(bindRequestComponents.size());
    bindRequestSequence.insert(bindRequestSequence.end(), lengthBytes.begin(), lengthBytes.end());
    
    bindRequestSequence.insert(bindRequestSequence.end(), 
                              bindRequestComponents.begin(), 
                              bindRequestComponents.end());
    
    // Combine messageID and bindRequestSequence into final result
    result.insert(result.end(), messageIDBytes.begin(), messageIDBytes.end());
    result.insert(result.end(), bindRequestSequence.begin(), bindRequestSequence.end());
    
    // Wrap everything in a SEQUENCE
    return encodeSequence(result);
}

std::vector<unsigned char> createLDAPSearchRequest(int messageID, const char* baseDN, const char* filter) {
    std::vector<unsigned char> result;
    
    // Message ID
    std::vector<unsigned char> messageIDBytes = encodeInteger(messageID);
    
    // Search Request components
    std::vector<unsigned char> searchRequestComponents;
    
    // Base DN (OCTET STRING)
    std::vector<unsigned char> baseDNBytes = encodeOctetString(baseDN);
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
    std::vector<unsigned char> attrBytes = encodeOctetString("cn");
    filterSeq.insert(filterSeq.end(), attrBytes.begin(), attrBytes.end());
    
    // Value: the search string
    std::vector<unsigned char> valueBytes = encodeOctetString(filter);
    filterSeq.insert(filterSeq.end(), valueBytes.begin(), valueBytes.end());
    
    // Add length to filter
    std::vector<unsigned char> lenBytes = encodeLength(filterSeq.size());
    filterBytes.insert(filterBytes.end(), lenBytes.begin(), lenBytes.end());
    
    // Add filter sequence to filterBytes
    filterBytes.insert(filterBytes.end(), filterSeq.begin(), filterSeq.end());
    
    // Add filter to search request
    searchRequestComponents.insert(searchRequestComponents.end(), filterBytes.begin(), filterBytes.end());
    
    // Attributes to return (SEQUENCE OF OCTET STRING)
    // For now, we'll request just "telephoneNumber"
    std::vector<unsigned char> attrsSequence;
    std::vector<unsigned char> telNoBytes = encodeOctetString("telephoneNumber");
    attrsSequence.insert(attrsSequence.end(), telNoBytes.begin(), telNoBytes.end());
    
    // Wrap attributes in a SEQUENCE
    std::vector<unsigned char> attrsWrapper = encodeSequence(attrsSequence);
    searchRequestComponents.insert(searchRequestComponents.end(), attrsWrapper.begin(), attrsWrapper.end());
    
    // Wrap search request components in a SEQUENCE with SEARCH REQUEST tag (0x63)
    std::vector<unsigned char> searchRequestSequence;
    searchRequestSequence.push_back(LDAP_SEARCH_REQUEST);  // SEARCH REQUEST tag
    
    std::vector<unsigned char> reqLenBytes = encodeLength(searchRequestComponents.size());
    searchRequestSequence.insert(searchRequestSequence.end(), reqLenBytes.begin(), reqLenBytes.end());
    
    searchRequestSequence.insert(searchRequestSequence.end(), 
                                searchRequestComponents.begin(), 
                                searchRequestComponents.end());
    
    // Combine messageID and searchRequestSequence into final result
    result.insert(result.end(), messageIDBytes.begin(), messageIDBytes.end());
    result.insert(result.end(), searchRequestSequence.begin(), searchRequestSequence.end());
    
    // Wrap everything in a SEQUENCE
    return encodeSequence(result);
}

// Simple BER parser for responses
class BERParser {
private:
    std::vector<unsigned char> data;
    size_t position;
    
public:
    BERParser(const std::vector<unsigned char>& data) : data(data), position(0) {}
    
    unsigned char readTag() {
        if (position >= data.size()) return 0;
        return data[position++];
    }
    
    size_t readLength() {
        if (position >= data.size()) return 0;
        
        unsigned char len = data[position++];
        
        if (len < 128) {
            return len;
        } else {
            int numBytes = len & 0x7F;
            size_t result = 0;
            
            for (int i = 0; i < numBytes; i++) {
                if (position >= data.size()) return 0;
                result = (result << 8) | data[position++];
            }
            
            return result;
        }
    }
    
    std::vector<unsigned char> readValue(size_t length) {
        std::vector<unsigned char> result;
        
        for (size_t i = 0; i < length && position < data.size(); i++) {
            result.push_back(data[position++]);
        }
        
        return result;
    }
    
    int readInteger() {
        unsigned char tag = readTag();
        if (tag != 0x02) return 0;  // Not an INTEGER
        
        size_t len = readLength();
        std::vector<unsigned char> value = readValue(len);
        
        int result = 0;
        for (unsigned char byte : value) {
            result = (result << 8) | byte;
        }
        
        return result;
    }
    
    std::string readOctetString() {
        unsigned char tag = readTag();
        if (tag != 0x04) return "";  // Not an OCTET STRING
        
        size_t len = readLength();
        std::vector<unsigned char> value = readValue(len);
        
        std::string result;
        for (unsigned char byte : value) {
            result += static_cast<char>(byte);
        }
        
        return result;
    }
    
    void skipEntry() {
        unsigned char tag = readTag();
        size_t len = readLength();
        position += len;
    }
    
    size_t getPosition() const {
        return position;
    }
    
    size_t getSize() const {
        return data.size();
    }
};

std::string parseLDAPSearchResponse(const std::vector<unsigned char>& response) {
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

class LDAPClient {
private:
    int sock;
    struct sockaddr_in server;
    int messageID;
    
public:
    LDAPClient() : sock(-1), messageID(1) {}
    
    bool connect(const char* host, int port) {
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
    
    bool bind(const char* bindDN, const char* password) {
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
    
    std::string search(const char* baseDN, const char* filter) {
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
    
    void close() {
        if (sock != -1) {
            ::close(sock);
            sock = -1;
        }
    }
    
    ~LDAPClient() {
        close();
    }
};

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
