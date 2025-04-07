// MAIN for PhoneBookServer

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <memory>
#include <regex>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
    #define CLOSE_SOCKET closesocket
#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    typedef int socket_t;
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

namespace fs = std::filesystem;

// MIME type mapping
const std::unordered_map<std::string, std::string> MIME_TYPES = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".txt", "text/plain"}
};

// Phone book entry structure
struct Contact {
    std::string name;
    std::string phone;
    std::string imagePath;

    Contact() = default;
    Contact(const std::string& n, const std::string& p, const std::string& i = "")
        : name(n), phone(p), imagePath(i) {}
};

// HTTP request parser
class HttpRequest {
    public:
        std::string method;
        std::string path;
        std::string httpVersion;
        std::map<std::string, std::string> headers;
        std::map<std::string, std::string> queryParams;
        std::string body;

        HttpRequest() = default;

        static HttpRequest parse(const std::string& requestStr) {
            HttpRequest request;
            std::istringstream stream(requestStr);
            std::string line;

            // Parse request line
            if (std::getline(stream, line)) {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                std::istringstream lineStream(line);
                lineStream >> request.method >> request.path >> request.httpVersion;
                
                // Parse query parameters if present
                size_t questionMarkPos = request.path.find('?');
                if (questionMarkPos != std::string::npos) {
                    std::string queryString = request.path.substr(questionMarkPos + 1);
                    request.path = request.path.substr(0, questionMarkPos);
                    request.parseQueryParams(queryString);
                }
            }

            // Parse headers
            while (std::getline(stream, line) && !line.empty() && line != "\r") {
                line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string key = line.substr(0, colonPos);
                    std::string value = line.substr(colonPos + 1);
                    // Trim leading/trailing whitespace
                    value.erase(0, value.find_first_not_of(' '));
                    value.erase(value.find_last_not_of(' ') + 1);
                    request.headers[key] = value;
                }
            }

            // Parse body if Content-Length is present
            if (request.headers.find("Content-Length") != request.headers.end()) {
                int contentLength = std::stoi(request.headers["Content-Length"]);
                if (contentLength > 0) {
                    char* bodyBuffer = new char[contentLength + 1];
                    stream.read(bodyBuffer, contentLength);
                    bodyBuffer[contentLength] = '\0';
                    request.body = bodyBuffer;
                    delete[] bodyBuffer;
                    
                    // If this is a form submission, parse the form data
                    if (request.headers["Content-Type"] == "application/x-www-form-urlencoded") {
                        request.parseQueryParams(request.body);
                    }
                }
            }

            return request;
        }

    private:
        void parseQueryParams(const std::string& queryString) {
            std::istringstream stream(queryString);
            std::string pair;
            
            while (std::getline(stream, pair, '&')) {
                size_t equalsPos = pair.find('=');
                if (equalsPos != std::string::npos) {
                    std::string key = pair.substr(0, equalsPos);
                    std::string value = pair.substr(equalsPos + 1);
                    // URL decode the value
                    queryParams[urlDecode(key)] = urlDecode(value);
                }
            }
        }
        
        std::string urlDecode(const std::string& encoded) {
            std::string result;
            for (size_t i = 0; i < encoded.length(); ++i) {
                if (encoded[i] == '%' && i + 2 < encoded.length()) {
                    int value;
                    std::istringstream(encoded.substr(i + 1, 2)) >> std::hex >> value;
                    result += static_cast<char>(value);
                    i += 2;
                } else if (encoded[i] == '+') {
                    result += ' ';
                } else {
                    result += encoded[i];
                }
            }
            return result;
        }
};

// HTTP response builder
class HttpResponse {
    public:
        int statusCode;
        std::string statusMessage;
        std::map<std::string, std::string> headers;
        std::string body;

        HttpResponse() : statusCode(200), statusMessage("OK") {
            headers["Server"] = "PhoneBookServer/1.0";
            headers["Connection"] = "close";
            setDate();
        }

        void setStatus(int code, const std::string& message) {
            statusCode = code;
            statusMessage = message;
        }

        void setContentType(const std::string& contentType) {
            headers["Content-Type"] = contentType;
        }

        void setContentLength() {
            headers["Content-Length"] = std::to_string(body.length());
        }

        void setDate() {
            std::time_t now = std::time(nullptr);
            std::tm* gmt = std::gmtime(&now);
            
            char buffer[100];
            std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
            
            headers["Date"] = buffer;
        }

        std::string toString() const {
            std::ostringstream response;
            response << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
            
            for (const auto& header : headers) {
                response << header.first << ": " << header.second << "\r\n";
            }
            
            response << "\r\n";
            response << body;
            
            return response.str();
        }
};

// Phone Book class that manages contacts
class PhoneBook {
    private:
        std::map<std::string, Contact> contacts;
        std::string imagesDir;

    public:
        PhoneBook(const std::string& imageDirectory = "images") : imagesDir(imageDirectory) {
            // Create images directory if it doesn't exist
            if (!fs::exists(imagesDir) && !imagesDir.empty()) {
                fs::create_directory(imagesDir);
            }
            
            // Add some sample contacts for testing
            addContact("John Doe", "123-456-7890");
            addContact("Jane Smith", "987-654-3210");
        }

        bool addContact(const std::string& name, const std::string& phone, const std::string& imagePath = "") {
            if (name.empty() || phone.empty()) {
                return false;
            }
            
            contacts[name] = Contact(name, phone, imagePath);
            return true;
        }

        bool deleteContact(const std::string& name) {
            auto it = contacts.find(name);
            if (it != contacts.end()) {
                // If the contact has an image, you might want to delete it as well
                if (!it->second.imagePath.empty() && fs::exists(it->second.imagePath)) {
                    fs::remove(it->second.imagePath);
                }
                contacts.erase(it);
                return true;
            }
            return false;
        }

        Contact* findContact(const std::string& name) {
            auto it = contacts.find(name);
            if (it != contacts.end()) {
                return &it->second;
            }
            return nullptr;
        }

        std::vector<Contact> getAllContacts() const {
            std::vector<Contact> result;
            for (const auto& pair : contacts) {
                result.push_back(pair.second);
            }
            return result;
        }

        std::string saveImage(const std::string& name, const std::string& imageData, const std::string& contentType) {
            // Extract file extension from content type
            std::string ext = ".jpg"; // Default
            if (contentType == "image/png") ext = ".png";
            else if (contentType == "image/gif") ext = ".gif";
            
            // Create a filename based on the contact name
            std::string sanitizedName = name;
            std::replace_if(sanitizedName.begin(), sanitizedName.end(), 
                        [](char c) { return !std::isalnum(c); }, '_');
            
            std::string filename = imagesDir + "/" + sanitizedName + ext;
            
            // Save the image
            std::ofstream file(filename, std::ios::binary);
            if (file) {
                file << imageData;
                file.close();
                return filename;
            }
            
            return "";
        }
};

// HTTP Server class
class HttpServer {
    private:
        socket_t serverSocket;
        int port;
        PhoneBook phoneBook;
        bool running;

    public:
        HttpServer(int serverPort = 8080) : port(serverPort), running(false) {
            // Initialize socket library on Windows
            #ifdef _WIN32
            WSADATA wsaData;
            if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
                throw std::runtime_error("Failed to initialize Winsock");
            }
            #endif

            // Create socket
            serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket == INVALID_SOCKET) {
                throw std::runtime_error("Failed to create socket");
            }

            // Enable address reuse
            int opt = 1;
            if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
                throw std::runtime_error("Failed to set socket options");
            }

            // Bind socket
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(port);

            if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
                CLOSE_SOCKET(serverSocket);
                throw std::runtime_error("Failed to bind socket");
            }

            // Listen for connections
            if (listen(serverSocket, 5) == SOCKET_ERROR) {
                CLOSE_SOCKET(serverSocket);
                throw std::runtime_error("Failed to listen on socket");
            }

            std::cout << "Server started on port " << port << std::endl;
        }

        ~HttpServer() {
            stop();
            
            CLOSE_SOCKET(serverSocket);
            
            #ifdef _WIN32
            WSACleanup();
            #endif
        }

        void start() {
            running = true;
            
            while (running) {
                sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                
                socket_t clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
                if (clientSocket == INVALID_SOCKET) {
                    std::cerr << "Failed to accept connection" << std::endl;
                    continue;
                }
                
                // Handle client connection in the same thread
                handleClient(clientSocket);
            }
        }

        void stop() {
            running = false;
        }

    private:
        void handleClient(socket_t clientSocket) {
            const int bufferSize = 8192;
            char buffer[bufferSize];
            std::string requestData;
            
            // Receive data from client
            int bytesReceived;
            while ((bytesReceived = recv(clientSocket, buffer, bufferSize - 1, 0)) > 0) {
                buffer[bytesReceived] = '\0';
                requestData.append(buffer, bytesReceived);
                
                // Check if we've received the end of the HTTP headers
                if (requestData.find("\r\n\r\n") != std::string::npos) {
                    break;
                }
            }
            
            if (bytesReceived < 0) {
                std::cerr << "Error receiving data from client" << std::endl;
                CLOSE_SOCKET(clientSocket);
                return;
            }
            
            // Parse the HTTP request
            HttpRequest request = HttpRequest::parse(requestData);
            
            // Handle the request
            HttpResponse response = routeRequest(request);
            
            // Send response to client
            std::string responseStr = response.toString();
            send(clientSocket, responseStr.c_str(), responseStr.length(), 0);
            
            // Close client socket
            CLOSE_SOCKET(clientSocket);
        }

        HttpResponse routeRequest(const HttpRequest& request) {
            HttpResponse response;
            
            // Route the request based on the path and method
            if (request.method == "GET") {
                if (request.path == "/" || request.path == "/index.html") {
                    serveIndexPage(response);
                } else if (request.path == "/search") {
                    handleSearch(request, response);
                } else if (request.path == "/images" && request.queryParams.find("name") != request.queryParams.end()) {
                    serveContactImage(request.queryParams.at("name"), response);
                } else if (request.path.find("/images/") == 0) {
                    // Serve a specific image file
                    serveFile(request.path.substr(1), response); // Remove leading slash
                } else {
                    // Try to serve it as a static file
                    std::string requestPath = request.path.substr(1); // Remove leading slash
                    if (!serveFile(requestPath, response)) {
                        response.setStatus(404, "Not Found");
                        response.setContentType("text/html");
                        response.body = "<html><body><h1>404 Not Found</h1><p>The requested resource was not found.</p></body></html>";
                    }
                }
            } else if (request.method == "POST") {
                if (request.path == "/add") {
                    handleAddContact(request, response);
                } else if (request.path == "/delete") {
                    handleDeleteContact(request, response);
                } else {
                    response.setStatus(405, "Method Not Allowed");
                    response.setContentType("text/html");
                    response.body = "<html><body><h1>405 Method Not Allowed</h1></body></html>";
                }
            } else {
                response.setStatus(501, "Not Implemented");
                response.setContentType("text/html");
                response.body = "<html><body><h1>501 Not Implemented</h1><p>The method is not implemented.</p></body></html>";
            }
            
            response.setContentLength();
            return response;
        }

        void serveIndexPage(HttpResponse& response) {
            std::vector<Contact> contacts = phoneBook.getAllContacts();
            
            std::ostringstream html;
            html << "<!DOCTYPE html>\n"
                << "<html>\n"
                << "<head>\n"
                << "    <title>Phone Book</title>\n"
                << "    <style>\n"
                << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
                << "        h1 { color: #333; }\n"
                << "        .contact { margin-bottom: 10px; padding: 10px; border: 1px solid #ddd; border-radius: 5px; }\n"
                << "        .contact img { max-width: 100px; max-height: 100px; margin-right: 10px; }\n"
                << "        form { margin-bottom: 20px; padding: 15px; background-color: #f8f8f8; border-radius: 5px; }\n"
                << "        label { display: inline-block; width: 80px; }\n"
                << "        input[type=text] { padding: 5px; margin: 5px; width: 200px; }\n"
                << "        input[type=submit] { padding: 5px 15px; background-color: #4CAF50; color: white; border: none; border-radius: 4px; cursor: pointer; }\n"
                << "        input[type=submit]:hover { background-color: #45a049; }\n"
                << "        .delete { background-color: #f44336; }\n"
                << "        .delete:hover { background-color: #d32f2f; }\n"
                << "    </style>\n"
                << "</head>\n"
                << "<body>\n"
                << "    <h1>Phone Book</h1>\n"
                << "    <h2>Add New Contact</h2>\n"
                << "    <form action=\"/add\" method=\"post\" enctype=\"multipart/form-data\">\n"
                << "        <div><label for=\"name\">Name:</label><input type=\"text\" id=\"name\" name=\"name\" required></div>\n"
                << "        <div><label for=\"phone\">Phone:</label><input type=\"text\" id=\"phone\" name=\"phone\" required></div>\n"
                << "        <div><input type=\"submit\" value=\"Add Contact\"></div>\n"
                << "    </form>\n"
                << "    <h2>Search Contact</h2>\n"
                << "    <form action=\"/search\" method=\"get\">\n"
                << "        <div><label for=\"q\">Name:</label><input type=\"text\" id=\"q\" name=\"q\" required></div>\n"
                << "        <div><input type=\"submit\" value=\"Search\"></div>\n"
                << "    </form>\n"
                << "    <h2>Contacts</h2>\n";
            
            for (const auto& contact : contacts) {
                html << "    <div class=\"contact\">\n";
                
                // Display contact image if available
                if (!contact.imagePath.empty() && fs::exists(contact.imagePath)) {
                    html << "        <img src=\"/images?name=" << contact.name << "\" alt=\"" << contact.name << "\">\n";
                }
                
                html << "        <strong>" << contact.name << "</strong>: " << contact.phone << "\n"
                    << "        <form action=\"/delete\" method=\"post\" style=\"display:inline;margin-left:10px;\">\n"
                    << "            <input type=\"hidden\" name=\"name\" value=\"" << contact.name << "\">\n"
                    << "            <input type=\"submit\" class=\"delete\" value=\"Delete\">\n"
                    << "        </form>\n"
                    << "    </div>\n";
            }
            
            html << "</body>\n"
                << "</html>";
            
            response.setContentType("text/html");
            response.body = html.str();
        }

        void handleSearch(const HttpRequest& request, HttpResponse& response) {
            std::string query;
            if (request.queryParams.find("q") != request.queryParams.end()) {
                query = request.queryParams.at("q");
            }
            
            Contact* contact = phoneBook.findContact(query);
            
            std::ostringstream html;
            html << "<!DOCTYPE html>\n"
                << "<html>\n"
                << "<head>\n"
                << "    <title>Search Results</title>\n"
                << "    <style>\n"
                << "        body { font-family: Arial, sans-serif; margin: 20px; }\n"
                << "        h1 { color: #333; }\n"
                << "        .contact { margin-bottom: 10px; padding: 10px; border: 1px solid #ddd; border-radius: 5px; }\n"
                << "        .contact img { max-width: 100px; max-height: 100px; margin-right: 10px; }\n"
                << "        a { color: #2196F3; text-decoration: none; }\n"
                << "        a:hover { text-decoration: underline; }\n"
                << "    </style>\n"
                << "</head>\n"
                << "<body>\n"
                << "    <h1>Search Results for \"" << query << "\"</h1>\n"
                << "    <a href=\"/\">Back to Phone Book</a>\n";
            
            if (contact) {
                html << "    <div class=\"contact\">\n";
                
                // Display contact image if available
                if (!contact->imagePath.empty() && fs::exists(contact->imagePath)) {
                    html << "        <img src=\"/images?name=" << contact->name << "\" alt=\"" << contact->name << "\">\n";
                }
                
                html << "        <strong>" << contact->name << "</strong>: " << contact->phone << "\n"
                    << "    </div>\n";
            } else {
                html << "    <p>No contact found with that name.</p>\n";
            }
            
            html << "</body>\n"
                << "</html>";
            
            response.setContentType("text/html");
            response.body = html.str();
        }

        void handleAddContact(const HttpRequest& request, HttpResponse& response) {
            std::string name, phone;
            
            if (request.queryParams.find("name") != request.queryParams.end()) {
                name = request.queryParams.at("name");
            }
            
            if (request.queryParams.find("phone") != request.queryParams.end()) {
                phone = request.queryParams.at("phone");
            }
            
            bool success = phoneBook.addContact(name, phone);
            
            // Redirect to the main page
            response.setStatus(302, "Found");
            response.headers["Location"] = "/";
            response.body = "";
        }

        void handleDeleteContact(const HttpRequest& request, HttpResponse& response) {
            std::string name;
            
            if (request.queryParams.find("name") != request.queryParams.end()) {
                name = request.queryParams.at("name");
                phoneBook.deleteContact(name);
            }
            
            // Redirect to the main page
            response.setStatus(302, "Found");
            response.headers["Location"] = "/";
            response.body = "";
        }

        void serveContactImage(const std::string& name, HttpResponse& response) {
            Contact* contact = phoneBook.findContact(name);
            
            if (contact && !contact->imagePath.empty() && fs::exists(contact->imagePath)) {
                // Get the file extension
                std::string ext = fs::path(contact->imagePath).extension().string();
                std::string contentType = "image/jpeg"; // Default
                
                auto it = MIME_TYPES.find(ext);
                if (it != MIME_TYPES.end()) {
                    contentType = it->second;
                }
                
                // Read the image file
                std::ifstream file(contact->imagePath, std::ios::binary);
                if (file) {
                    // Get file size
                    file.seekg(0, std::ios::end);
                    size_t fileSize = file.tellg();
                    file.seekg(0, std::ios::beg);
                    
                    // Read file content
                    std::vector<char> buffer(fileSize);
                    file.read(buffer.data(), fileSize);
                    
                    response.setContentType(contentType);
                    response.body.assign(buffer.data(), fileSize);
                    return;
                }
            }
            
            // If contact not found or no image
            response.setStatus(404, "Not Found");
            response.setContentType("text/plain");
            response.body = "Image not found";
        }

        bool serveFile(const std::string& path, HttpResponse& response) {
            // Don't allow directory traversal
            if (path.find("..") != std::string::npos) {
                return false;
            }
            
            // Check if file exists
            if (!fs::exists(path) || !fs::is_regular_file(path)) {
                return false;
            }
            
            // Get the file extension and determine MIME type
            std::string ext = fs::path(path).extension().string();
            std::string contentType = "application/octet-stream"; // Default
            
            auto it = MIME_TYPES.find(ext);
            if (it != MIME_TYPES.end()) {
                contentType = it->second;
            }
            
            // Read the file
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                return false;
            }
            
            // Get file size
            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            
            // Read file content
            std::vector<char> buffer(fileSize);
            file.read(buffer.data(), fileSize);
            
            response.setContentType(contentType);
            response.body.assign(buffer.data(), fileSize);
            
            return true;
        }
};

int main() {
    try {
        HttpServer server(8080);
        std::cout << "Phone Book Server started on port 8080" << std::endl;
        std::cout << "Open your browser and navigate to http://localhost:8080" << std::endl;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
