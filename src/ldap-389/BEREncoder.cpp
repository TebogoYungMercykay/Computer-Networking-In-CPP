#include "BEREncoder.h"
#include <cstring>

std::vector<unsigned char> BEREncoder::encodeLength(size_t len) {
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

std::vector<unsigned char> BEREncoder::encodeInteger(int value) {
    std::vector<unsigned char> result;
    
    // Type: INTEGER (0x02)
    result.push_back(0x02);
    
    // For simplicity, we'll handle only small positive integers
    result.push_back(0x01);  // Length: 1 byte
    result.push_back(static_cast<unsigned char>(value & 0xFF));
    
    return result;
}

std::vector<unsigned char> BEREncoder::encodeOctetString(const char* str) {
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

std::vector<unsigned char> BEREncoder::encodeSequence(const std::vector<unsigned char>& data) {
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
