#include <iostream>

int main() {
    std::cout << "Content-type: text/html\n\n";

    std::cout << "<!DOCTYPE html>\n";
    std::cout << "<html lang=\"en\">\n";
    std::cout << "<head>\n";
    std::cout << "    <meta charset=\"UTF-8\">\n";
    std::cout << "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    std::cout << "    <meta name=\"author\" content=\"Your Name\">\n";
    std::cout << "    <meta name=\"description\" content=\"A simple C++ CGI script\">\n";
    std::cout << "    <title>C++ CGI Script</title>\n";
    std::cout << "    <style>\n";
    std::cout << "        body { font-family: Arial, sans-serif; background-color: #f4f4f4; text-align: center; padding: 50px; }\n";
    std::cout << "        h1 { color: #333; }\n";
    std::cout << "    </style>\n";
    std::cout << "</head>\n";
    std::cout << "<body>\n";
    std::cout << "    <h1>Hello from C++ CGI!</h1>\n";
    std::cout << "    <p>This is a properly formatted HTML5 page served by a C++ CGI script.</p>\n";
    std::cout << "</body>\n";
    std::cout << "</html>\n";

    return 0;
}
