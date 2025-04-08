#include "BERParser.h"

BERParser::BERParser(const std::vector<unsigned char>& data) : data(data), position(0) {}

unsigned char BERParser::readTag() {
    if (position >= data.size()) return 0;
    return data[position++];
}

size_t BERParser::readLength() {
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

std::vector<unsigned char> BERParser::readValue(size_t length) {
    std::vector<unsigned char> result;
    
    for (size_t i = 0; i < length && position < data.size(); i++) {
        result.push_back(data[position++]);
    }
    
    return result;
}

int BERParser::readInteger() {
    unsigned char tag = readTag();
    // Not an INTEGER
    if (tag != 0x02) return 0;
    
    size_t len = readLength();
    std::vector<unsigned char> value = readValue(len);
    
    int result = 0;
    for (unsigned char byte : value) {
        result = (result << 8) | byte;
    }
    
    return result;
}

std::string BERParser::readOctetString() {
    unsigned char tag = readTag();
    // Not an OCTET STRING
    if (tag != 0x04) return "";
    
    size_t len = readLength();
    std::vector<unsigned char> value = readValue(len);
    
    std::string result;
    for (unsigned char byte : value) {
        result += static_cast<char>(byte);
    }
    
    return result;
}

void BERParser::skipEntry() {
    unsigned char tag = readTag();
    size_t len = readLength();
    position += len;
}

size_t BERParser::getPosition() const {
    return position;
}

size_t BERParser::getSize() const {
    return data.size();
}
