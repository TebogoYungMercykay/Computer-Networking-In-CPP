#include <iostream>
#include <fstream>

int main() {
    std::ofstream backendFile("/var/www/data/timezone.txt");
    
    if (backendFile.is_open()) {
        backendFile << "2\n";
        backendFile << "South Africa\n";
        backendFile << "Pretoria\n";
        backendFile.close();
    }

    std::cout << "Content-type: text/html\n\n";
    
    std::cout << "<!DOCTYPE html>\n";
    std::cout << "<html lang=\"en\">\n";
    std::cout << "<head>\n";
    std::cout << "    <meta charset=\"UTF-8\">\n";
    std::cout << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    std::cout << "    <meta http-equiv=\"refresh\" content=\"0; url=/cgi-bin/showtime.cgi\">\n";
    std::cout << "    <title>Set Time Zone</title>\n";
    std::cout << "</head>\n";
    std::cout << "<body>\n";
    std::cout << "    <p>LOADING...</p>\n";
    std::cout << "</body>\n";
    std::cout << "</html>\n";
    
    return 0;
}
