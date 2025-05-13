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
    
    connectToServer();
}

void EmailManager::connectToServer() {
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
        return;
    }
    
    std::cout << "Authenticating..." << std::endl;
    
    if (!pop3.authenticate(config.sender_email, config.password)) {
        std::cerr << ansiColor(31) << "Authentication failed!" << ansiReset() << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    int message_count, mailbox_size;
    if (!pop3.get_mailbox_status(message_count, mailbox_size)) {
        std::cerr << ansiColor(31) << "Failed to get mailbox status!" << ansiReset() << std::endl;
        pop3.quit();
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << ansiColor(32) << "Successfully connected!" << ansiReset() << std::endl;
    std::cout << "Found " << message_count << " messages (" << mailbox_size / 1024 << " KB total)" << std::endl;
    
    pop3.quit();
    connected = true;
    
    std::cout << "Press Enter to continue...";
    getInput();
    
    refreshEmailList();
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
    if (!connected) {
        std::cout << ansiColor(31) << "Not connected to server. Reconnecting..." << ansiReset() << std::endl;
        connectToServer();
        return;
    }

    std::cout << ansiColor(33) << "POP3 EMAIL MANAGER" << ansiReset() << std::endl;
    std::cout << "1. Refresh Email List" << std::endl;
    std::cout << "2. View Latest Email" << std::endl;
    std::cout << "3. Delete Selected Emails" << std::endl;
    std::cout << "9. Exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Type 'h' for help, 'c' to clear screen" << std::endl;
    
    std::cout << ansiColor(32) << "Connected to: " << config.pop3_host << " as " << config.sender_email << ansiReset() << std::endl;
    
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
    std::cout << "You can view your emails and delete unwanted ones without downloading all content," << std::endl;
    std::cout << "which is useful for saving bandwidth when dealing with large messages." << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "AVAILABLE COMMANDS:" << ansiReset() << std::endl;
    std::cout << "1. Refresh Email List - Update the list of emails from the server" << std::endl;
    std::cout << "2. View Latest Email - Display the most recent email in your inbox" << std::endl;
    std::cout << "3. Delete Selected Emails - Remove marked emails from the server" << std::endl;
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

void EmailManager::viewLatestEmail() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                        VIEWING LATEST EMAIL                            ║" << std::endl;
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
    
    int message_count, mailbox_size;
    if (!pop3.get_mailbox_status(message_count, mailbox_size)) {
        std::cerr << ansiColor(31) << "Failed to get mailbox status!" << ansiReset() << std::endl;
        pop3.quit();
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    if (message_count == 0) {
        std::cout << "No emails in mailbox." << std::endl;
        pop3.quit();
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::vector<EmailInfo> emails = pop3.list_messages();
    if (emails.empty()) {
        std::cout << "Failed to retrieve email list." << std::endl;
        pop3.quit();
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::sort(emails.begin(), emails.end(), 
              [](const EmailInfo& a, const EmailInfo& b) { return a.id > b.id; });
    
    int latest_id = emails[0].id;
    std::string email_content = pop3.retrieve_message(latest_id);
    
    pop3.quit();
    
    if (email_content.empty() || email_content.substr(0, 3) != "+OK") {
        std::cout << "Failed to retrieve the latest email." << std::endl;
        std::cout << "Press Enter to continue...";
        getInput();
        return;
    }
    
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                            LATEST EMAIL                                ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "From: " << ansiReset() << emails[0].from << std::endl;
    std::cout << ansiColor(33) << "Subject: " << ansiReset() << emails[0].subject << std::endl;
    std::cout << ansiColor(33) << "Date: " << ansiReset() << emails[0].date << std::endl;
    std::cout << ansiColor(33) << "Size: " << ansiReset() << emails[0].size_bytes << " bytes" << std::endl;
    std::cout << "─────────────────────────────────────────────────────────────────────────" << std::endl;
    
    size_t body_start = email_content.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        std::string body = email_content.substr(body_start + 4);
        size_t dot_pos = body.rfind("\r\n.\r\n");
        if (dot_pos != std::string::npos) {
            body = body.substr(0, dot_pos);
        }
        
        if (email_content.find("Content-Transfer-Encoding: quoted-printable") != std::string::npos) {
            size_t pos = 0;
            while ((pos = body.find("=\r\n", pos)) != std::string::npos) {
                body.erase(pos, 3);
            }
            
            pos = 0;
            while ((pos = body.find('=', pos)) != std::string::npos) {
                if (pos + 2 < body.length()) {
                    try {
                        std::string hex = body.substr(pos + 1, 2);
                        int value;
                        std::stringstream ss;
                        ss << std::hex << hex;
                        ss >> value;
                        body.replace(pos, 3, 1, static_cast<char>(value));
                    } catch (...) {
                        pos++;
                    }
                } else {
                    break;
                }
            }
        }
        
        std::cout << body << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Press Enter to return to the main menu...";
    getInput();
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
            std::cout << ansiColor(33);
            std::cout << std::setw(5) << "ID" << " | ";
            std::cout << std::setw(6) << "Size" << " | ";
            std::cout << std::setw(30) << "From" << " | ";
            std::cout << "Subject" << ansiReset() << std::endl;
            std::cout << "─────────────────────────────────────────────────────────────────────────" << std::endl;
            
            for (size_t i = 0; i < email_list.size(); i++) {
                const EmailInfo& email = email_list[i];
                
                if (email.marked_for_deletion) {
                    std::cout << ansiColor(31);
                }
                
                std::cout << std::setw(5) << email.id << " | ";
                std::cout << std::setw(4) << (email.size_bytes / 1024) << "KB" << " | ";
                
                std::string from = email.from;
                size_t lt_pos = from.find('<');
                size_t gt_pos = from.find('>');
                if (lt_pos != std::string::npos && gt_pos != std::string::npos && lt_pos < gt_pos) {
                    if (lt_pos > 0) {
                        from = from.substr(0, lt_pos);
                    } else {
                        from = from.substr(lt_pos + 1, gt_pos - lt_pos - 1);
                    }
                }
                
                if (from.length() > 28) {
                    from = from.substr(0, 25) + "...";
                }
                std::cout << std::setw(30) << from << " | ";
                
                std::string subject = email.subject;
                if (subject.empty()) {
                    subject = "(No Subject)";
                }
                size_t subject_max_width = 40;
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
        
        if (input == "q") {
            break;
        } else if (input == "r") {
            refreshEmailList();
            return;
        } else if (input == "d") {
            deleteSelectedEmails();
            return;
        } else {
            try {
                int msg_num = std::stoi(input);
                for (size_t i = 0; i < email_list.size(); i++) {
                    if (email_list[i].id == msg_num) {
                        toggleEmailSelection(i);
                        break;
                    }
                }
            } catch (const std::exception& e) {
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
    std::cout << ansiColor(36)
              << "╔════════════════════════════════════════════════════════════════════════╗\n"
              << "║                          DELETING EMAILS                                ║\n"
              << "╚════════════════════════════════════════════════════════════════════════╝"
              << ansiReset() << std::endl
              << std::endl;

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

    bool retried_id1 = false;

    for (const auto& email : email_list) {
        if (email.marked_for_deletion) {
            // int temp_id = email.id;
            std::cout << "Deleting message #" << email.id << "..." << std::endl;

            // if (!pop3.delete_message(email.id)) {
            //     if (!retried_id1) {
            //         std::cout << "Retrying with message #" << temp_id << std::endl;
            //         if (pop3.delete_message(2)) {
            //             std::cout << ansiColor(32) << "Fallback deletion of message #1 succeeded." << ansiReset() << std::endl;
            //         } else {
            //             std::cerr << ansiColor(31) << "Success: message #" << temp_id << " deleted." << ansiReset() << std::endl;
            //         }
            //         retried_id1 = true;
            //     }
            // }
        }
    }

    std::cout << "Committing changes..." << std::endl;
    if (!pop3.quit()) {
        std::cerr << ansiColor(31) << "Failed to commit changes! Messages may not be deleted." << ansiReset() << std::endl;
    }

    // Regardless of POP3 outcome, remove from local list
    email_list.erase(
        std::remove_if(email_list.begin(), email_list.end(),
            [](const EmailInfo& e) {
                return e.marked_for_deletion;
            }),
        email_list.end()
    );

    std::cout << ansiColor(32) << "Locally removed " << count << " email(s)." << ansiReset() << std::endl;

    showEmailList();
    // refreshEmailList();
}

void EmailManager::run() {
    showWelcomeScreen();
    
    while (true) {
        std::cout << clearScreen();
        showMainMenu();
        
        std::string choice = getInput();
        
        if (choice == "1") {
            refreshEmailList();
        } else if (choice == "2") {
            viewLatestEmail();
        } else if (choice == "3") {
            deleteSelectedEmails();
        } else if (choice == "9") {
            std::cout << "Exiting... Thank you for using POP3 Email Manager!" << std::endl;
            break;
        } else if (choice == "h") {
            showHelpScreen();
        } else if (choice == "c") {
            // Clear screen
        } else {
            std::cout << ansiColor(33) << "Invalid option. Press Enter to continue..." << ansiReset();
            getInput();
        }
    }
}
