#include <iostream>
#include <fstream>
#include <ctime>
#include <string>

int main() {
    std::ifstream backendFile("/var/www/data/timezone.txt");
    int timezoneOffset = 2; 
    std::string countryName = "South Africa";
    std::string capitalCity = "Pretoria";
    
    if (backendFile.is_open()) {
        backendFile >> timezoneOffset;
        
        
        if (!backendFile.eof()) {
            backendFile.ignore(); 
            std::getline(backendFile, countryName);
            
            if (!backendFile.eof()) {
                std::getline(backendFile, capitalCity);
            }
        }
        
        backendFile.close();
    }
    
    time_t now = time(0);
    struct tm *gmtTime = gmtime(&now);
    
    gmtTime->tm_hour += timezoneOffset;
    mktime(gmtTime);
    
    char timeString[100];
    strftime(timeString, sizeof(timeString), "%H:%M:%S", gmtTime);
    
    std::cout << "Content-type: text/html\n\n";
    
    std::cout << "<!DOCTYPE html>\n";
    std::cout << "<html lang=\"en\">\n";
    std::cout << "<head>\n";
    std::cout << "    <meta charset=\"UTF-8\">\n";
    std::cout << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    std::cout << "    <title>Current Time</title>\n";
    std::cout << "    <style>\n";
    std::cout << "        body { font-family: Arial, sans-serif; background-color: #f4f4f4; text-align: center; padding: 50px; }\n";
    std::cout << "        h1 { color: #333; }\n";
    std::cout << "        .links { margin-top: 20px; }\n";
    std::cout << "        .links a { margin: 0 10px; }\n";
    std::cout << "    </style>\n";
    std::cout << "</head>\n";
    std::cout << "<body>\n";
    std::cout << "    <h1>Current Time Display</h1>\n";
    std::cout << "    <p>The current time in " << capitalCity << ", " << countryName << " is " << timeString << "</p>\n";
    std::cout << "    <div class=\"links\">\n";
    std::cout << "        <a href=\"/cgi-bin/setsa.cgi\">Switch to South African Time</a>\n";
    std::cout << "        <a href=\"/cgi-bin/setgh.cgi\">Switch to Ghana Time</a>\n";
    std::cout << "    </div>\n";
    std::cout << "</body>\n";
    std::cout << "</html>\n";
    
    return 0;
}
