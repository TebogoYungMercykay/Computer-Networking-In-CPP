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

        auto handler = []() {
            std::cout << ansiColor(92) << "Handler: File was successfully uploaded!" << ansiReset() << std::endl;
        };

        FileMonitor monitor(config, handler);
        monitor.monitor();

        return 0;
    }
    catch (const std::exception& e) {
        std::cout << ansiColor(91) << "Fatal error: " << e.what() << ansiReset() << std::endl;
        return 1;
    }
}
