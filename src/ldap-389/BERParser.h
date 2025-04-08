// BERParser - Handles parsing of BER-encoded responses.

#ifndef BER_PARSER_H
#define BER_PARSER_H

#include <vector>
#include <string>
#include <cstddef>

class BERParser {
    private:
        std::vector<unsigned char> data;
        size_t position;
        
    public:
        BERParser(const std::vector<unsigned char>& data);
        
        unsigned char readTag();
        size_t readLength();
        std::vector<unsigned char> readValue(size_t length);
        int readInteger();
        std::string readOctetString();
        void skipEntry();
        
        size_t getPosition() const;
        size_t getSize() const;
};

#endif // BER_PARSER_H
