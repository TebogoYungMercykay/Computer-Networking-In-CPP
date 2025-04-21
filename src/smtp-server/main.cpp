#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "alarm_system.h"
#include "utils.h"

int main() {
    // Initialize OpenSSL for the entire application
    SSL_library_init();
    SSL_load_error_strings();
    
    try {
        AlarmSystem alarm_system;
        alarm_system.run();
    }
    catch (const std::exception& e) {
        std::cerr << ansiColor(31) << "Error: " << e.what() << ansiReset() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << ansiColor(31) << "Unknown error occurred!" << ansiReset() << std::endl;
        return 1;
    }
    
    // Clean up OpenSSL resources
    EVP_cleanup();
    
    return 0;
}
