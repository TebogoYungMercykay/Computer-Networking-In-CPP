#include "LDAPClient.h"
#include "Contact.h"
#include <iostream>
#include <string>
#include <vector>
#include <limits>

// ANSI color codes and terminal control sequences
std::string ansiColor(int color) {
    return "\033[" + std::to_string(color) + "m";
}

std::string ansiReset() {
    return "\033[0m";
}

std::string clearScreen() {
    return "\033[2J\033[H";
}

void showWelcomeScreen() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║           ██╗     ██████╗  █████╗ ██████╗      ██████╗███╗   ███╗      ║" << std::endl;
    std::cout << "║           ██║     ██╔══██╗██╔══██╗██╔══██╗    ██╔════╝████╗ ████║      ║" << std::endl;
    std::cout << "║           ██║     ██║  ██║███████║██████╔╝    ██║     ██╔████╔██║      ║" << std::endl;
    std::cout << "║           ██║     ██║  ██║██╔══██║██╔═══╝     ██║     ██║╚██╔╝██║      ║" << std::endl;
    std::cout << "║           ███████╗██████╔╝██║  ██║██║         ╚██████╗██║ ╚═╝ ██║      ║" << std::endl;
    std::cout << "║           ╚══════╝╚═════╝ ╚═╝  ╚═╝╚═╝          ╚═════╝╚═╝     ╚═╝      ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║            Welcome to the LDAP Contact Management System!              ║" << std::endl;
    std::cout << "║            Connect, manage, and retrieve your contacts easily.         ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
}

void showMainMenu() {
    std::cout << ansiColor(33) << "MAIN MENU:" << ansiReset() << std::endl;
    std::cout << "1. Search for a contact" << std::endl;
    std::cout << "2. Advanced search" << std::endl;
    std::cout << "3. Exit" << std::endl;
    // std::cout << "3. List all contacts" << std::endl;
    std::cout << std::endl;
    std::cout << "Type 'h' for help, 'c' to clear screen" << std::endl;
    std::cout << ansiColor(32) << "Enter your choice: " << ansiReset();
}

void showHelp() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                              HELP MENU                                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    std::cout << "This application uses LDAP protocol to query a directory of contacts." << std::endl;
    std::cout << "Here's what you can do:" << std::endl;
    std::cout << std::endl;
    std::cout << "1. Search for a contact - Search by name to find contact information" << std::endl;
    std::cout << "2. List all contacts - Display all contacts in the directory" << std::endl;
    std::cout << "3. Advanced search - Search using specific attributes (name, phone number)" << std::endl;
    std::cout << "4. Exit - Close the application" << std::endl;
    std::cout << std::endl;
    std::cout << "Additional commands:" << std::endl;
    std::cout << "  'h' - Show this help screen" << std::endl;
    std::cout << "  'c' - Clear the screen" << std::endl;
    std::cout << std::endl;
    std::cout << ansiColor(33) << "Press Enter to return to the main menu..." << ansiReset();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void displaySearchResult(const std::string& name, const std::string& phoneNumber) {
    if (!phoneNumber.empty()) {
        std::cout << ansiColor(32) << "✓ " << ansiReset();
        std::cout << "Contact found: " << ansiColor(36) << name << ansiReset() << std::endl;
        std::cout << "  Phone number: " << ansiColor(33) << phoneNumber << ansiReset() << std::endl;
        std::cout << std::endl;
    } else {
        std::cout << ansiColor(31) << "✗ " << ansiReset();
        std::cout << "No contact found with name: " << ansiColor(36) << name << ansiReset() << std::endl;
        std::cout << std::endl;
    }
}

void displayContacts(const std::vector<Contact>& contacts) {
    if (contacts.empty()) {
        std::cout << ansiColor(31) << "✗ " << ansiReset();
        std::cout << "No contacts found." << std::endl;
        std::cout << std::endl;
        return;
    }

    std::cout << ansiColor(32) << "✓ " << ansiReset();
    std::cout << "All Contacts:" << std::endl;
    
    for (const auto& contact : contacts) {
        std::cout << "  * " << ansiColor(36) << (contact.name.empty() ? "[No Name]" : contact.name) << ansiReset() << ": " 
                  << ansiColor(33) << (contact.phoneNumber.empty() ? "[No Phone]" : contact.phoneNumber) << ansiReset() << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Total: " << contacts.size() << " contact(s)" << std::endl;
    std::cout << std::endl;
}

void searchContact(LDAPClient& client) {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                           CONTACT SEARCH                               ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::string contactName;
    std::cout << "Enter contact name to search (or 'back' to return): ";
    std::getline(std::cin, contactName);
    
    if (contactName == "back") {
        return;
    }
    
    std::cout << ansiColor(33) << "Searching for contact: " << contactName << ansiReset() << std::endl;
    std::cout << "Please wait..." << std::endl;
    
    // Search for the contact's telephone number
    std::string phoneNumber = client.search("ou=Friends,dc=friends,dc=local", contactName.c_str());
    
    displaySearchResult(contactName, phoneNumber);
    
    std::cout << ansiColor(33) << "Press Enter to continue..." << ansiReset();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void listAllContacts(LDAPClient& client) {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                           ALL CONTACTS                                 ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "Retrieving all contacts..." << ansiReset() << std::endl;
    
    // Retrieve all contacts using the searchAll method
    std::vector<Contact> contacts = client.searchAll("ou=Friends,dc=friends,dc=local");
    
    // Display the contacts
    displayContacts(contacts);
    
    std::cout << ansiColor(33) << "Press Enter to continue..." << ansiReset();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

void advancedSearch(LDAPClient& client) {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                         ADVANCED SEARCH                                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::string nameFilter, phoneFilter;
    
    std::cout << "Enter name to search for (leave empty to ignore): ";
    std::getline(std::cin, nameFilter);
    
    std::cout << "Enter phone number to search for (leave empty to ignore): ";
    std::getline(std::cin, phoneFilter);
    
    if (nameFilter.empty() && phoneFilter.empty()) {
        std::cout << ansiColor(33) << "At least one search criteria is needed." << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    
    std::cout << ansiColor(33) << "Searching for contacts..." << ansiReset() << std::endl;
    std::cout << "Please wait..." << std::endl;
    
    // Perform advanced search using name and/or phone filters
    std::vector<Contact> results = client.advancedSearch("ou=Friends,dc=friends,dc=local", nameFilter, phoneFilter);
    
    // Display the search criteria used
    std::cout << "Search criteria:" << std::endl;
    if (!nameFilter.empty()) {
        std::cout << "  Name contains: " << ansiColor(36) << nameFilter << ansiReset() << std::endl;
    }
    if (!phoneFilter.empty()) {
        std::cout << "  Phone contains: " << ansiColor(36) << phoneFilter << ansiReset() << std::endl;
    }
    std::cout << std::endl;
    
    // Display the results
    displayContacts(results);
    
    std::cout << ansiColor(33) << "Press Enter to continue..." << ansiReset();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int main() {
    LDAPClient client;
    
    // Connect to OpenLDAP server
    if (!client.connect("127.0.0.1", 389)) {
        std::cerr << "Failed to connect to LDAP server!" << std::endl;
        return 1;
    }
    
    // Bind to LDAP server (anonymous bind in this case)
    if (!client.bind("", "")) {
        std::cerr << "Failed to bind to LDAP server!" << std::endl;
        return 1;
    }
    
    showWelcomeScreen();
    std::cout << "Press Enter to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    
    bool running = true;
    std::string choice;
    
    while (running) {
        std::cout << clearScreen();
        showMainMenu();
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            searchContact(client);
        } else if (choice == "2") {
            advancedSearch(client);
        } else if (choice == "3") {
            running = false;
        // } else if (choice == "4") {
        //     listAllContacts(client);
        } else if (choice == "h") {
            showHelp();
        } else if (choice == "c") {
            std::cout << clearScreen();
        } else {
            std::cout << ansiColor(31) << "Invalid choice. Press Enter to continue..." << ansiReset();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "Thank you for using the LDAP Contact Management System!" << ansiReset() << std::endl;
    
    client.close();
    return 0;
}
