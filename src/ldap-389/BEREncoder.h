// BEREncoder - Handles all ASN.1 BER encoding functionality.

#ifndef BER_ENCODER_H
#define BER_ENCODER_H

#include <vector>
#include <cstddef>
#include <string>

class BEREncoder {
    public:
        static std::vector<unsigned char> encodeInteger(int value);
        static std::vector<unsigned char> encodeOctetString(const char* str);
        static std::vector<unsigned char> encodeSequence(const std::vector<unsigned char>& data);
        static std::vector<unsigned char> encodeLength(size_t len);
};

#endif // BER_ENCODER_H
