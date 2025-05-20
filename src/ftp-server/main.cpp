#include "Config.h"
#include "FileMonitor.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    try {
        std::string envPath = ".env";
        
        // Allow specifying a different .env file via command line
        if (argc > 1) {
            envPath = argv[1];
        }
        
        // Load configuration
        Config config = loadConfig(envPath);
        
        // Create and start file monitor
        FileMonitor monitor(config);
        monitor.monitor();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
