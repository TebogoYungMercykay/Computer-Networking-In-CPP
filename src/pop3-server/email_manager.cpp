#include "email_manager.h"
#include "pop3_client.h"
#include "utils.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <algorithm>

EmailManager::EmailManager() {
    config = load_config();
    current_selection = 0;
    connected = false;
    
    // Validate POP3 configuration
    if (config.pop3_host.empty() || config.sender_email.empty() || config.password.empty()) {
        std::cerr << ansiColor(31) << "Error: Incomplete POP3 configuration. Please check your .env file." << ansiReset() << std::endl;
        exit(EXIT_FAILURE);
    }
}

void EmailManager::showWelcomeScreen() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║                  ██████╗  ██████╗ ██████╗ ██████╗                     ║" << std::endl;
    std::cout << "║                  ██╔══██╗██╔═══██╗██╔══██╗╚════██╗                    ║" << std::endl;
    std::cout << "║                  ██████╔╝██║   ██║██████╔╝ █████╔╝                    ║" << std::endl;
    std::cout << "║                  ██╔═══╝ ██║   ██║██╔═══╝  ╚═══██╗                    ║" << std::endl;
    std::cout << "║                  ██║     ╚██████╔╝██║     ██████╔╝                    ║" << std::endl;
    std::cout << "║                  ╚═╝      ╚═════╝ ╚═╝     ╚═════╝                     ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║                   Email Manager & Cleanup Utility                      ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
}

void EmailManager::showMainMenu() {
    std::cout << ansiColor(33) << "POP3 EMAIL MANAGER" << ansiReset() << std::endl;
    std::cout << "1. Connect to Mailbox" << std::endl;
    if (connected) {
        std::cout << "2. Refresh Email List" << std::endl;
        std::cout << "3. Delete Selected Emails" << std::endl;
        std::cout << "4. Disconnect" << std::endl;
    }
    std::cout << "9. Exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Type 'h' for help, 'c' to clear screen" << std::endl;
    
    if (connected) {
        std::cout << ansiColor(32) << "Connected to: " << config.pop3_host << " as " << config.sender_email << ansiReset() << std::endl;
    }
    
    std::cout << ansiColor(32) << "Enter your choice: " << ansiReset();
}

void EmailManager::showHelpScreen() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                             HELP INFORMATION                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    std::cout << ansiColor(33) << "ABOUT THIS PROGRAM:" << ansiReset() << std::endl;
    std::cout << "This program allows you to manage your email mailbox using the POP3 protocol." << std::endl;
    std::cout << "You can view your emails and delete unwanted ones without downloading them," << std::endl;
    std::cout << "which is useful for saving bandwidth when dealing with large messages." << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "AVAILABLE COMMANDS:" << ansiReset() << std::endl;
    std::cout << "1. Connect to Mailbox - Connect to your email server" << std::endl;
    std::cout << "2. Refresh Email List - Update the list of emails" << std::endl;
    std::cout << "3. Delete Selected Emails - Remove marked emails from the server" << std::endl;
    std::cout << "4. Disconnect - Close the connection to the email server" << std::endl;
    std::cout << "9. Exit - Close the application" << std::endl;
    std::cout << "h - Display this help screen" << std::endl;
    std::cout << "c - Clear the screen" << std::endl;
    std::cout << std::endl;
    
    std::cout << "When viewing emails:" << std::endl;
    std::cout << "- Use number keys to toggle selection of emails" << std::endl;
    std::cout << "- Press 'd' to delete selected emails" << std::endl;
    std::cout << "- Press 'r' to refresh the email list" << std::endl;
    std::cout << "- Press 'q' to return to the main menu" << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "CONFIGURATION:" << ansiReset() << std::endl;
    std::cout << "The system uses the following email configuration:" << std::endl;
    std::cout << "- POP3 Server: " << config.pop3_host << ":" << config.pop3_port << std::endl;
    std::cout << "- Email Account: " << config.sender_email << std::endl;
    std::cout << std::endl;
    
    std::cout << "Press Enter to return to the main menu...";
    getInput();
    std::cout << clearScreen();
}

bool EmailManager::connectToMailbox() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                        CONNECTING TO MAILBOX                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::cout << "Connecting to " << config.pop3_host << ":" << config.pop3_port << " as " << config.sender_email << "..." << std::endl;
    
    POP3Client pop3(config.pop3_use_ssl);
    
    if (!pop3.connect_to_server(config.pop3_host, config.pop3_port)) {
        std::cerr << ansiColor(31) << "Failed to connect to POP3 server!" << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return false;
    }
    
    std::cout << "Authenticating..." << std::endl;
    
    if (!pop3.authenticate(config.sender_email, config.password)) {
        std::cerr << ansiColor(31) << "Authentication failed!" << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return false;
    }
    
    int message_count, mailbox_size;
    if (!pop3.get_mailbox_status(message_count, mailbox_size)) {
        std::cerr << ansiColor(31) << "Failed to get mailbox status!" << ansiReset() << std::endl;
        pop3.quit();
        std::cout << "Press Enter to continue...";
        getInput();
        return false;
    }
    
    std::cout << ansiColor(32) << "Successfully connected!" << ansiReset() << std::endl;
    std::cout << "Found " << message_count << " messages (" << mailbox_size / 1024 << " KB total)" << std::endl;
    
    pop3.quit();
    connected = true;
    
    std::cout << "Press Enter to continue...";
    getInput();
    
    refreshEmailList();
    return true;
}

void EmailManager::refreshEmailList() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                        RETRIEVING EMAIL LIST                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::cout << "Connecting to mailbox..." << std::endl;
    
    POP3Client pop3(config.pop3_use_ssl);
    
    if (!pop3.connect_to_server(config.pop3_host, config.pop3_port)) {
        std::cerr << ansiColor(31) << "Failed to connect to POP3 server!" << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << "Authenticating..." << std::endl;
    
    if (!pop3.authenticate(config.sender_email, config.password)) {
        std::cerr << ansiColor(31) << "Authentication failed!" << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << "Retrieving message list..." << std::endl;
    
    email_list = pop3.list_messages();
    
    pop3.quit();
    
    // Sort emails by ID
    std::sort(email_list.begin(), email_list.end(), 
              [](const EmailInfo& a, const EmailInfo& b) { return a.id < b.id; });
    
    std::cout << ansiColor(32) << "Retrieved " << email_list.size() << " emails." << ansiReset() << std::endl;
    
    if (!email_list.empty()) {
        showEmailList();
    } else {
        std::cout << "No emails in mailbox." << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
    }
}

void EmailManager::showEmailList() {
    while (true) {
        std::cout << clearScreen();
        std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║                            EMAIL LIST                                  ║" << std::endl;
        std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
        std::cout << std::endl;
        
        if (email_list.empty()) {
            std::cout << "No emails in mailbox." << std::endl;
        } else {
            // Display column headers
            std::cout << ansiColor(33);
            std::cout << std::setw(5) << "ID" << " | ";
            std::cout << std::setw(6) << "Size" << " | ";
            std::cout << std::setw(30) << "From" << " | ";
            std::cout << "Subject" << ansiReset() << std::endl;
            std::cout << "─────────────────────────────────────────────────────────────────────────" << std::endl;
            
            // Display emails
            for (size_t i = 0; i < email_list.size(); i++) {
                const EmailInfo& email = email_list[i];
                
                // Highlight selected emails
                if (email.marked_for_deletion) {
                    std::cout << ansiColor(31); // Red for marked items
                }
                
                std::cout << std::setw(5) << email.id << " | ";
                std::cout << std::setw(4) << (email.size_bytes / 1024) << "KB" << " | ";
                
                // Truncate long sender names
                std::string from = email.from;
                if (from.length() > 28) {
                    from = from.substr(0, 25) + "...";
                }
                std::cout << std::setw(30) << from << " | ";
                
                // Truncate subject if needed
                std::string subject = email.subject;
                if (subject.empty()) {
                    subject = "(No Subject)";
                }
                // Limit to available width
                size_t subject_max_width = 50;
                if (subject.length() > subject_max_width) {
                    subject = subject.substr(0, subject_max_width - 3) + "...";
                }
                std::cout << subject;
                
                if (email.marked_for_deletion) {
                    std::cout << ansiReset();
                }
                
                std::cout << std::endl;
            }
        }
        
        std::cout << std::endl;
        std::cout << "Commands: [#]=Toggle Selection, [d]=Delete Selected, [r]=Refresh, [q]=Back to Main Menu" << std::endl;
        std::cout << ansiColor(32) << "Enter command: " << ansiReset();
        
        std::string input = getInput();
        
        // Process commands
        if (input == "q") {
            break;
        } else if (input == "r") {
            refreshEmailList();
            return;
        } else if (input == "d") {
            deleteSelectedEmails();
            return;
        } else {
            // Try to parse as a message number
            try {
                int msg_num = std::stoi(input);
                // Find the message in our list
                for (size_t i = 0; i < email_list.size(); i++) {
                    if (email_list[i].id == msg_num) {
                        toggleEmailSelection(i);
                        break;
                    }
                }
            } catch (const std::exception& e) {
                // Not a number, ignore
            }
        }
    }
}

void EmailManager::toggleEmailSelection(int index) {
    if (index >= 0 && index < static_cast<int>(email_list.size())) {
        email_list[index].marked_for_deletion = !email_list[index].marked_for_deletion;
    }
}

void EmailManager::deleteSelectedEmails() {
    // Count selected emails
    int count = 0;
    for (const auto& email : email_list) {
        if (email.marked_for_deletion) {
            count++;
        }
    }
    
    if (count == 0) {
        std::cout << ansiColor(33) << "No emails selected for deletion." << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                          DELETING EMAILS                                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::cout << "You have selected " << count << " email(s) for deletion." << std::endl;
    std::cout << ansiColor(31) << "WARNING: This action cannot be undone!" << ansiReset() << std::endl;
    std::cout << "Proceed with deletion? (y/n): ";
    
    std::string confirm = getInput();
    if (confirm != "y" && confirm != "Y") {
        std::cout << "Deletion cancelled." << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << "Connecting to mailbox..." << std::endl;
    
    POP3Client pop3(config.pop3_use_ssl);
    
    if (!pop3.connect_to_server(config.pop3_host, config.pop3_port)) {
        std::cerr << ansiColor(31) << "Failed to connect to POP3 server!" << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << "Authenticating..." << std::endl;
    
    if (!pop3.authenticate(config.sender_email, config.password)) {
        std::cerr << ansiColor(31) << "Authentication failed!" << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << "Deleting emails..." << std::endl;
    
    int deleted = 0;
    for (const auto& email : email_list) {
        if (email.marked_for_deletion) {
            std::cout << "Deleting message #" << email.id << "..." << std::endl;
            if (pop3.delete_message(email.id)) {
                deleted++;
            } else {
                std::cerr << ansiColor(31) << "Failed to delete message #" << email.id << ansiReset() << std::endl;
            }
        }
    }
    
    // Commit the changes
    if (deleted > 0) {
        std::cout << "Committing changes..." << std::endl;
        if (!pop3.quit()) {
            std::cerr << ansiColor(31) << "Failed to commit changes! Messages may not be deleted." << ansiReset() << std::endl;
        } else {
            std::cout << ansiColor(32) << "Successfully deleted " << deleted << " email(s)." << ansiReset() << std::endl;
        }
    } else {
        pop3.quit();
    }
    
    std::cout << "Press Enter to continue...";
    getInput();
    
    // Refresh the email list after deletion
    refreshEmailList();
}

void EmailManager::run() {
    showWelcomeScreen();
    
    while (true) {
        std::cout << clearScreen();
        showMainMenu();
        
        std::string choice = getInput();
        
        if (choice == "1") {
            // Connect to Mailbox
            connectToMailbox();
        } else if (choice == "2" && connected) {
            // Refresh Email List
            refreshEmailList();
        } else if (choice == "3" && connected) {
            // Delete Selected Emails
            deleteSelectedEmails();
        } else if (choice == "4" && connected) {
            // Disconnect
            connected = false;
            std::cout << "Disconnected from mailbox." << std::endl;
            std::cout << "Press Enter to continue...";
            getInput();
        } else if (choice == "9") {
            // Exit
            std::cout << "Exiting... Thank you for using POP3 Email Manager!" << std::endl;
            break;
        } else if (choice == "h") {
            // Help
            showHelpScreen();
        } else if (choice == "c") {
            // Clear screen - already handled in the next loop iteration
        } else {
            std::cout << ansiColor(33) << "Invalid option. Press Enter to continue..." << ansiReset();
            getInput();
        }
    }
}
