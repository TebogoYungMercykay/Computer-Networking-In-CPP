#include "Config.h"
#include "FileMonitor.h"
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    try {
        std::string envPath = ".env";
        
        if (argc > 1) {
            envPath = argv[1];
        }
        
        Config config = loadConfig(envPath);
        FileMonitor monitor(config);

        monitor.monitor();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
