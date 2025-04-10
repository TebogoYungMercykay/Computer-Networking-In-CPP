#ifndef OPENSSL_BASE64_H
#define OPENSSL_BASE64_H

#include <string>

std::string base64_encode(const std::string& input);
std::string base64_decode(const std::string& input);

#endif // OPENSSL_BASE64_H
