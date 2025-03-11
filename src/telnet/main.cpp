#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <thread>
#include <mutex>
#include <memory>

// Socket headers
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
    #include <fcntl.h>
    typedef int socket_t;
    #define CLOSE_SOCKET close
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

const int PORT = 8080;
const std::string DATABASE_FILE = "phonebook.txt";
std::mutex contactsMutex;

class Contact {
    private:
        std::string name;
        std::string phoneNumber;

    public:
        Contact(const std::string& name, const std::string& phoneNumber)
            : name(name), phoneNumber(phoneNumber) {}

        std::string getName() const { return name; }
        std::string getPhoneNumber() const { return phoneNumber; }
        void setPhoneNumber(const std::string& newNumber) { phoneNumber = newNumber; }

        std::string toString() const {
            return name + ": " + phoneNumber;
        }
};

// Global contacts database
std::vector<Contact> contacts;

// Function prototypes
void loadContactsFromFile();
void saveContactsToFile();
void handleClient(socket_t clientSocket);
std::string readLine(socket_t socket);
void sendLine(socket_t socket, const std::string& line);

// ANSI escape sequence utilities
std::string clearScreen() {
    return "\x1B[2J\x1B[1;1H";
}

std::string moveCursor(int row, int col) {
    return "\x1B[" + std::to_string(row) + ";" + std::to_string(col) + "H";
}

std::string ansiColor(int colorCode) {
    return "\x1B[" + std::to_string(colorCode) + "m";
}

std::string ansiReset() {
    return "\x1B[0m";
}

int main() {
    // Load contacts from file
    loadContactsFromFile();

#ifdef _WIN32
    // Initialize Winsock for Windows
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }
#endif

    // Create server socket
    socket_t serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // Setup server address structure
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed" << std::endl;
        CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // Listen for connections
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed" << std::endl;
        CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    std::cout << "Phone Book Server started on port " << PORT << std::endl;
    std::cout << "Waiting for clients to connect..." << std::endl;

    // Accept connections and handle clients
    while (true) {
        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        
        socket_t clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed" << std::endl;
            continue;
        }

        // Get client IP address
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected: " << clientIP << std::endl;

        // Create a new thread to handle the client
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    // Clean up
    CLOSE_SOCKET(serverSocket);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

void loadContactsFromFile() {
    std::ifstream file(DATABASE_FILE);
    if (!file.is_open()) {
        std::cout << "Created new database file: " << DATABASE_FILE << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t pos = line.find(':');
        if (pos != std::string::npos) {
            std::string name = line.substr(0, pos);
            std::string phoneNumber = line.substr(pos + 1);
            
            // Trim whitespace
            name.erase(0, name.find_first_not_of(" \t"));
            name.erase(name.find_last_not_of(" \t") + 1);
            phoneNumber.erase(0, phoneNumber.find_first_not_of(" \t"));
            phoneNumber.erase(phoneNumber.find_last_not_of(" \t") + 1);
            
            contacts.emplace_back(name, phoneNumber);
        }
    }
    file.close();
    std::cout << "Loaded " << contacts.size() << " contacts from database" << std::endl;
}

void saveContactsToFile() {
    std::ofstream file(DATABASE_FILE);
    if (!file.is_open()) {
        std::cerr << "Error saving contacts: Could not open file" << std::endl;
        return;
    }
    
    for (const auto& contact : contacts) {
        file << contact.getName() << ":" << contact.getPhoneNumber() << std::endl;
    }
    file.close();
    std::cout << "Saved " << contacts.size() << " contacts to database" << std::endl;
}

std::string readLine(socket_t socket) {
    std::string result;
    char buffer[1];
    ssize_t bytesRead;

    while (true) {
        bytesRead = recv(socket, buffer, 1, 0);
        if (bytesRead <= 0) {
            return result;
        }

        if (buffer[0] == '\r' || buffer[0] == '\n') {
            if (buffer[0] == '\r') {
                recv(socket, buffer, 1, 0);
            }
            break;
        }
        
        result += buffer[0];
    }

    return result;
}

void sendLine(socket_t socket, const std::string& line) {
    std::string data = line + "\r\n";
    send(socket, data.c_str(), data.size(), 0);
}

void showWelcomeScreen(socket_t clientSocket) {
    sendLine(clientSocket, clearScreen());
    sendLine(clientSocket, ansiColor(36) + "╔════════════════════════════════════════════════════════════════════════╗");
    sendLine(clientSocket, "║                                                                        ║");
    sendLine(clientSocket, "║           ████████╗███████╗██╗     ███╗   ██╗███████╗████████╗         ║");
    sendLine(clientSocket, "║           ╚══██╔══╝██╔════╝██║     ████╗  ██║██╔════╝╚══██╔══╝         ║");
    sendLine(clientSocket, "║              ██║   █████╗  ██║     ██╔██╗ ██║█████╗     ██║            ║");
    sendLine(clientSocket, "║              ██║   ██╔══╝  ██║     ██║╚██╗██║██╔══╝     ██║            ║");
    sendLine(clientSocket, "║              ██║   ███████╗███████╗██║ ╚████║███████╗   ██║            ║");
    sendLine(clientSocket, "║              ╚═╝   ╚══════╝╚══════╝╚═╝  ╚═══╝╚══════╝   ╚═╝            ║");
    sendLine(clientSocket, "║                                                                        ║");
    sendLine(clientSocket, "║            Welcome to the Telnet Contact Management Server!            ║");
    sendLine(clientSocket, "║            Connect, manage, and retrieve your contacts easily.         ║");
    sendLine(clientSocket, "║                                                                        ║");
    sendLine(clientSocket, "╚════════════════════════════════════════════════════════════════════════╝" + ansiReset());
    sendLine(clientSocket, "");
}


void showMainMenu(socket_t clientSocket) {
    sendLine(clientSocket, ansiColor(33) + "MAIN MENU:" + ansiReset());
    sendLine(clientSocket, "1. List all contacts");
    sendLine(clientSocket, "2. Search for a contact");
    sendLine(clientSocket, "3. Add a new contact");
    sendLine(clientSocket, "4. Edit a contact");
    sendLine(clientSocket, "5. Delete a contact");
    sendLine(clientSocket, "6. Exit");
    sendLine(clientSocket, "");
    sendLine(clientSocket, "Type 'h' for help, 'c' to clear screen");
    sendLine(clientSocket, ansiColor(32) + "Enter your choice: " + ansiReset());
}

void showHelp(socket_t clientSocket) {
    sendLine(clientSocket, clearScreen());
    sendLine(clientSocket, ansiColor(33) + "HELP INFORMATION:" + ansiReset());
    sendLine(clientSocket, "This application allows you to manage your phone book contacts.");
    sendLine(clientSocket, "");
    sendLine(clientSocket, "Available commands:");
    sendLine(clientSocket, "  1-6        - Select menu options");
    sendLine(clientSocket, "  m          - Display main menu");
    sendLine(clientSocket, "  h          - Display this help screen");
    sendLine(clientSocket, "  c          - Clear the screen");
    sendLine(clientSocket, "  search NAME - Search for contacts by name");
    sendLine(clientSocket, "  add NAME:NUMBER - Add a new contact");
    sendLine(clientSocket, "  delete NAME - Delete a contact by name");
    sendLine(clientSocket, "");
    sendLine(clientSocket, "Press Enter to return to main menu...");
    std::string dummy = readLine(clientSocket);
    showMainMenu(clientSocket);
}

void listAllContacts(socket_t clientSocket) {
    std::unique_lock<std::mutex> lock(contactsMutex);
    
    sendLine(clientSocket, clearScreen());
    sendLine(clientSocket, ansiColor(33) + "ALL CONTACTS:" + ansiReset());
    sendLine(clientSocket, "");
    
    if (contacts.empty()) {
        sendLine(clientSocket, "No contacts found in the phone book.");
    } else {
        int index = 1;
        for (const auto& contact : contacts) {
            sendLine(clientSocket, std::to_string(index) + ". " + 
                     ansiColor(36) + contact.getName() + ansiReset() + 
                     " - " + ansiColor(35) + contact.getPhoneNumber() + ansiReset());
            index++;
        }
    }
    
    lock.unlock();
    sendLine(clientSocket, "");
    sendLine(clientSocket, "Press Enter to return to main menu...");
    std::string dummy = readLine(clientSocket);
    showMainMenu(clientSocket);
}

void searchContact(socket_t clientSocket, const std::string& query) {
    std::unique_lock<std::mutex> lock(contactsMutex);
    
    sendLine(clientSocket, clearScreen());
    sendLine(clientSocket, ansiColor(33) + "SEARCH RESULTS FOR '" + query + "':" + ansiReset());
    sendLine(clientSocket, "");
    
    bool found = false;
    for (const auto& contact : contacts) {
        std::string name = contact.getName();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        
        std::string queryLower = query;
        std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
        
        if (name.find(queryLower) != std::string::npos) {
            sendLine(clientSocket, ansiColor(36) + contact.getName() + ansiReset() + 
                     " - " + ansiColor(35) + contact.getPhoneNumber() + ansiReset());
            found = true;
        }
    }
    
    if (!found) {
        sendLine(clientSocket, "No matching contacts found.");
    }
    
    lock.unlock();
    sendLine(clientSocket, "");
    sendLine(clientSocket, "Press Enter to return to main menu...");
    std::string dummy = readLine(clientSocket);
    showMainMenu(clientSocket);
}

void addContact(socket_t clientSocket, const std::string& name, const std::string& phoneNumber) {
    std::unique_lock<std::mutex> lock(contactsMutex);
    
    // Check if contact already exists
    for (const auto& contact : contacts) {
        std::string existingName = contact.getName();
        std::transform(existingName.begin(), existingName.end(), existingName.begin(), ::tolower);
        
        std::string newName = name;
        std::transform(newName.begin(), newName.end(), newName.begin(), ::tolower);
        
        if (existingName == newName) {
            sendLine(clientSocket, "");
            sendLine(clientSocket, "Contact with name '" + name + "' already exists.");
            sendLine(clientSocket, "");
            sendLine(clientSocket, "Press Enter to return to main menu...");
            std::string dummy = readLine(clientSocket);
            showMainMenu(clientSocket);
            return;
        }
    }
    
    // Add new contact
    contacts.emplace_back(name, phoneNumber);
    saveContactsToFile();

    lock.unlock();
    sendLine(clientSocket, "");
    sendLine(clientSocket, ansiColor(32) + "Contact added successfully: " + ansiReset() + 
             ansiColor(36) + name + ansiReset() + " - " + 
             ansiColor(35) + phoneNumber + ansiReset());
    sendLine(clientSocket, "");
    sendLine(clientSocket, "Press Enter to return to main menu...");
    std::string dummy = readLine(clientSocket);
    showMainMenu(clientSocket);
}

void editContact(socket_t clientSocket, const std::string& name) {
    std::unique_lock<std::mutex> lock(contactsMutex);
    
    bool found = false;
    for (auto& contact : contacts) {
        std::string existingName = contact.getName();
        std::transform(existingName.begin(), existingName.end(), existingName.begin(), ::tolower);
        
        std::string searchName = name;
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
        
        if (existingName == searchName) {
            found = true;
            sendLine(clientSocket, "Enter new phone number for " + 
                     ansiColor(36) + contact.getName() + ansiReset() + ": ");
            
            std::string newPhoneNumber = readLine(clientSocket);
            contact.setPhoneNumber(newPhoneNumber);
            saveContactsToFile();
            
            sendLine(clientSocket, "");
            sendLine(clientSocket, ansiColor(32) + "Contact updated successfully!" + ansiReset());
            break;
        }
    }
    
    if (!found) {
        sendLine(clientSocket, "No contact found with name '" + name + "'.");
    }
    
    sendLine(clientSocket, "");
    lock.unlock();
    sendLine(clientSocket, "Press Enter to return to main menu...");
    std::string dummy = readLine(clientSocket);
    showMainMenu(clientSocket);
}

void deleteContact(socket_t clientSocket, const std::string& name) {
    std::unique_lock<std::mutex> lock(contactsMutex);
    
    bool found = false;
    for (auto it = contacts.begin(); it != contacts.end(); ++it) {
        std::string existingName = it->getName();
        std::transform(existingName.begin(), existingName.end(), existingName.begin(), ::tolower);
        
        std::string searchName = name;
        std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
        
        if (existingName == searchName) {
            contacts.erase(it);
            saveContactsToFile();
            found = true;
            break;
        }
    }
    
    if (found) {
        sendLine(clientSocket, "");
        sendLine(clientSocket, ansiColor(32) + "Contact '" + name + "' deleted successfully!" + ansiReset());
    } else {
        sendLine(clientSocket, "");
        sendLine(clientSocket, "No contact found with name '" + name + "'.");
    }
    
    sendLine(clientSocket, "");
    lock.unlock();
    sendLine(clientSocket, "Press Enter to return to main menu...");
    std::string dummy = readLine(clientSocket);
    showMainMenu(clientSocket);
}

void handleClient(socket_t clientSocket) {
    // Welcome the user and show menu
    showWelcomeScreen(clientSocket);
    showMainMenu(clientSocket);
    
    std::string inputLine;
    while (true) {
        inputLine = readLine(clientSocket);
        
        // Handle client disconnection
        if (inputLine.empty() && errno != 0) {
            break;
        }
        
        // Process commands
        if (inputLine.empty()) {
            showMainMenu(clientSocket);
            continue;
        }
        try {
            if (inputLine == "1") {
                listAllContacts(clientSocket);
            } else if (inputLine == "2") {
                sendLine(clientSocket, "");
                sendLine(clientSocket, "Enter search term: ");
                std::string query = readLine(clientSocket);
                searchContact(clientSocket, query);
            } else if (inputLine == "3") {
                sendLine(clientSocket, "");
                sendLine(clientSocket, "Enter new contact (Name:PhoneNumber): ");

                std::string contactInfo = readLine(clientSocket);
                size_t pos = contactInfo.find(':');
                if (pos != std::string::npos) {
                    std::string name = contactInfo.substr(0, pos);
                    std::string phoneNumber = contactInfo.substr(pos + 1);
                    
                    // Trim whitespace
                    name.erase(0, name.find_first_not_of(" \t"));
                    name.erase(name.find_last_not_of(" \t") + 1);
                    phoneNumber.erase(0, phoneNumber.find_first_not_of(" \t"));
                    phoneNumber.erase(phoneNumber.find_last_not_of(" \t") + 1);
                    
                    addContact(clientSocket, name, phoneNumber);
                } else {
                    sendLine(clientSocket, "Invalid format. Use: Name:PhoneNumber");
                }
            } else if (inputLine == "4") {
                sendLine(clientSocket, "");
                if (contacts.empty()) {
                    sendLine(clientSocket, "No contacts in the phone book to edit.");
                    sendLine(clientSocket, "Press Enter to return to main menu...");
                    std::string dummy = readLine(clientSocket);
                    showMainMenu(clientSocket);
                    continue;
                }
                
                listAllContacts(clientSocket);
                sendLine(clientSocket, "Enter the name of the contact to edit: ");
                std::string name = readLine(clientSocket);
                editContact(clientSocket, name);
            } else if (inputLine == "5") {
                sendLine(clientSocket, "");
                if (contacts.empty()) {
                    sendLine(clientSocket, "No contacts in the phone book to delete.");
                    sendLine(clientSocket, "Press Enter to return to main menu...");
                    std::string dummy = readLine(clientSocket);
                    showMainMenu(clientSocket);
                    continue;
                }
                
                sendLine(clientSocket, "Enter the name of the contact to delete: ");
                std::string name = readLine(clientSocket);
                deleteContact(clientSocket, name);
            } else if (inputLine == "6") {
                sendLine(clientSocket, "");
                sendLine(clientSocket, ansiColor(32) + "Thank you for using the Phone Book Server!");
                sendLine(clientSocket, "Disconnecting..." + ansiReset());
                break;
            } else if (inputLine == "m") {
                showMainMenu(clientSocket);
            } else if (inputLine == "h") {
                showHelp(clientSocket);
            } else if (inputLine == "c") {
                sendLine(clientSocket, clearScreen());
                showMainMenu(clientSocket);
            } else if (inputLine.substr(0, 7) == "search ") {
                std::string query = inputLine.substr(7);
                searchContact(clientSocket, query);
            } else if (inputLine.substr(0, 4) == "add ") {
                std::string contactInfo = inputLine.substr(4);
                size_t pos = contactInfo.find(':');
                if (pos != std::string::npos) {
                    std::string name = contactInfo.substr(0, pos);
                    std::string phoneNumber = contactInfo.substr(pos + 1);
                    
                    // Trim whitespace
                    name.erase(0, name.find_first_not_of(" \t"));
                    name.erase(name.find_last_not_of(" \t") + 1);
                    phoneNumber.erase(0, phoneNumber.find_first_not_of(" \t"));
                    phoneNumber.erase(phoneNumber.find_last_not_of(" \t") + 1);
                    
                    addContact(clientSocket, name, phoneNumber);
                } else {
                    sendLine(clientSocket, "Invalid format. Use: add Name:PhoneNumber");
                }
            } else if (inputLine.substr(0, 7) == "delete ") {
                std::string name = inputLine.substr(7);
                deleteContact(clientSocket, name);
            } else {
                sendLine(clientSocket, "Unknown command. Type 'h' for help or 'm' for menu.");
            }
        } catch (const std::exception& e) {
            sendLine(clientSocket, "An error occurred: " + std::string(e.what()));
            sendLine(clientSocket, "");
            sendLine(clientSocket, "Press Enter to return to main menu...");
            std::string dummy = readLine(clientSocket);
            showMainMenu(clientSocket);
        }
    }
    
    // Close client socket
    CLOSE_SOCKET(clientSocket);
    std::cout << "Client disconnected" << std::endl;
}
