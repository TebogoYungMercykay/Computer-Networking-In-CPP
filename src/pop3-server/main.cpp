#include <iostream>
#include <string>
#include <vector>
#include "config.h"
#include "utils.h"
#include "email_manager.h"

void print_usage() {
    std::cout << "Usage: ./program [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --email      Start the email manager" << std::endl;
    std::cout << "  --help       Display this help message" << std::endl;
}

int main(int argc, char* argv[]) {
    EmailManager email_manager;
    email_manager.run();

    return 0;
}
