#include "openssl_base64.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdexcept>
#include <vector>

std::string base64_encode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    if (BIO_write(bio, input.data(), input.size()) <= 0) {
        BIO_free_all(bio);
        throw std::runtime_error("Failed to encode Base64");
    }
    BIO_flush(bio);

    // Get the encoded data
    BUF_MEM* buffer_ptr;
    BIO_get_mem_ptr(bio, &buffer_ptr);
    std::string output(buffer_ptr->data, buffer_ptr->length - 1);

    BIO_free_all(bio);
    return output;
}

std::string base64_decode(const std::string& input) {
    BIO* bio = BIO_new_mem_buf(input.data(), input.size());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  // ✅ Ignore newlines (important)
    bio = BIO_push(b64, bio);

    // Estimate decoded size (Base64 expands size by ~33%)
    std::vector<char> buffer(input.size());  

    int decoded_size = BIO_read(bio, buffer.data(), buffer.size());
    if (decoded_size <= 0) {
        BIO_free_all(bio);
        throw std::runtime_error("Failed to decode Base64");
    }

    BIO_free_all(bio);
    return std::string(buffer.data(), decoded_size);  // ✅ Correctly resize output
}

// OPENSSL_BASE64_CPP
