#include "alarm_system.h"
#include "smtp_client.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

// Loading configuration from .env file
Config load_config() {
    Config config;    
    std::ifstream env_file(".env");
    std::string line;
    
    if (!env_file.is_open()) {
        std::cerr << ansiColor(31) << "Warning: .env file not found. Using default values." << ansiReset() << std::endl;
        return config;
    }
    
    while (std::getline(env_file, line)) {
        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                // Removing quotes if present
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }
                
                if (key == "HOST") config.host = value;
                else if (key == "PORT") config.port = std::stoi(value);
                else if (key == "SENDER_EMAIL") config.sender_email = value;
                else if (key == "EMAIL_PASSWORD") config.password = value;
                else if (key == "RECEIVER_EMAIL") config.receiver_email = value;
                else if (key == "USE_SSL") config.use_ssl = (value == "true" || value == "1");
            }
        }
    }
    
    return config;
}

// ---- AlarmSystem Class Implementation ----

AlarmSystem::AlarmSystem() {
    config = load_config();
    current_recipient_email = config.receiver_email;
    
    // Check if configuration is valid
    if (config.host.empty() || config.sender_email.empty() || config.password.empty()) {
        std::cerr << ansiColor(31) << "Error: Incomplete configuration. Please check your .env file." << ansiReset() << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // If no recipient email is configured, prompt for one
    if (current_recipient_email.empty()) {
        changeRecipientEmail();
    }
}

void AlarmSystem::showWelcomeScreen() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║                  ███████╗███╗   ███╗████████╗██████╗                   ║" << std::endl;
    std::cout << "║                  ██╔════╝████╗ ████║╚══██╔══╝██╔══██╗                  ║" << std::endl;
    std::cout << "║                  ███████╗██╔████╔██║   ██║   ██████╔╝                  ║" << std::endl;
    std::cout << "║                  ╚════██║██║╚██╔╝██║   ██║   ██╔═══╝                   ║" << std::endl;
    std::cout << "║                  ███████║██║ ╚═╝ ██║   ██║   ██║                       ║" << std::endl;
    std::cout << "║                  ╚══════╝╚═╝     ╚═╝   ╚═╝   ╚═╝                       ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║              Welcome to the SMTP Client Alert System!                  ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
}

void AlarmSystem::showMainMenu() {
    std::cout << ansiColor(33) << "Current time: " << getCurrentTimeString() << "\n\n";
    std::cout << "MAIN MENU:" << ansiReset() << std::endl;
    std::cout << "1. Send Email Alert" << std::endl;
    std::cout << "2. Change Recipient Email" << std::endl;
    std::cout << "3. Exit" << std::endl;
    std::cout << std::endl;
    std::cout << "Type 'h' for help, 'c' to clear screen" << std::endl;
    
    std::cout << ansiColor(90) << "Current recipient: " << current_recipient_email << ansiReset() << std::endl;
    std::cout << ansiColor(32) << "Enter your choice: " << ansiReset();
}

void AlarmSystem::showHelpScreen() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                             HELP INFORMATION                           ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    std::cout << ansiColor(33) << "ABOUT THIS PROGRAM:" << ansiReset() << std::endl;
    std::cout << "This program simulates a home security system that sends email alerts when" << std::endl;
    std::cout << "sensors are triggered. The program uses raw SMTP protocol to send emails" << std::endl;
    std::cout << "directly through a mail server." << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "AVAILABLE COMMANDS:" << ansiReset() << std::endl;
    std::cout << "1. Send Email Alert - Send alert email for the home security system" << std::endl;
    std::cout << "2. Change Recipient Email - Update the email address that will receive alerts" << std::endl;
    std::cout << "3. Exit - Close the application" << std::endl;
    std::cout << "h - Display this help screen" << std::endl;
    std::cout << "c - Clear the screen" << std::endl;
    std::cout << std::endl;

    std::cout << "'q' - Return to main menu" << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "CONFIGURATION:" << ansiReset() << std::endl;
    std::cout << "The system uses the following email configuration:" << std::endl;
    std::cout << "- SMTP Server: " << config.host << ":" << config.port << std::endl;
    std::cout << "- Sender Email: " << config.sender_email << std::endl;
    std::cout << "- Current Recipient: " << current_recipient_email << std::endl;
    std::cout << std::endl;
    
    std::cout << "Press Enter to return to the main menu...";
    getInput();
    std::cout << clearScreen();
}


std::string AlarmSystem::getCurrentTimeString() {
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void AlarmSystem::changeRecipientEmail() {
    std::cout << clearScreen();
    std::cout << ansiColor(36) << "╔════════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                        CHANGE RECIPIENT EMAIL                          ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
    
    std::cout << "Current recipient email: " << current_recipient_email << std::endl;
    std::cout << "Enter new recipient email (or leave empty to keep current): ";
    
    std::string new_email = getInput();
    if (!new_email.empty()) {
        // Basic email validation
        size_t at_pos = new_email.find('@');
        size_t dot_pos = new_email.rfind('.');
        
        if (at_pos == std::string::npos || dot_pos == std::string::npos || 
            at_pos > dot_pos || at_pos == 0 || dot_pos == new_email.length() - 1) {
            std::cout << ansiColor(31) << "Invalid email format! Email not changed." << ansiReset() << std::endl;
        } else {
            current_recipient_email = new_email;
            std::cout << ansiColor(32) << "Recipient email updated successfully!" << ansiReset() << std::endl;
        }
    } else {
        std::cout << "Email unchanged." << std::endl;
    }
    
    std::cout << "Press Enter to continue...";
    getInput();
    std::cout << clearScreen();
}

void AlarmSystem::triggerAlert() {
    std::cout << ansiColor(36) << "\nSending alert email to " << current_recipient_email << "..." << ansiReset() << std::endl;
    
    std::string current_time = getCurrentTimeString();
    std::string subject = "Email Alert from Home Security System";
    std::string body = 
        "========== EMAIL ALERT ==========\r\n\r\n"
        "This is a Email alert from your home security system.\r\n"
        "Time: " + current_time + "\r\n\r\n"
        "If you received this email, your security system is properly configured "
        "and can send alerts in case of an emergency.\r\n\r\n"
        "==============================\r\n"
        "Home Security System";
    
    if (sendAlertEmail(subject, body)) {
        std::cout << ansiColor(32) << "Email alert sent successfully!" << ansiReset() << std::endl;
    }

    std::cout << "Press Enter to continue...";
    getInput();
    std::cout << clearScreen();
}

bool AlarmSystem::sendAlertEmail(const std::string& subject, const std::string& body) {
    std::cout << ansiColor(33) << "Connecting to SMTP server..." << ansiReset() << std::endl;
    
    SMTPClient smtp(config.use_ssl);
    
    // Connect to SMTP server
    if (!smtp.connect_to_server(config.host, config.port)) {
        std::cerr << ansiColor(31) << "Failed to connect to SMTP server!" << ansiReset() << std::endl;
        return false;
    }
    
    std::cout << ansiColor(33) << "Authenticating..." << ansiReset() << std::endl;
    
    // Authenticate with the SMTP server
    if (!smtp.authenticate(config.sender_email, config.password)) {
        std::cerr << ansiColor(31) << "Authentication failed!" << ansiReset() << std::endl;
        return false;
    }
    
    std::cout << ansiColor(33) << "Sending email..." << ansiReset() << std::endl;
    
    // Send the email
    if (!smtp.send_email(config.sender_email, current_recipient_email, subject, body)) {
        std::cerr << ansiColor(31) << "Failed to send email!" << ansiReset() << std::endl;
        return false;
    }
    
    smtp.quit();
    
    std::cout << ansiColor(32) << "Email alert sent successfully!" << ansiReset() << std::endl;
    return true;
}

void AlarmSystem::run() {
    showWelcomeScreen();
    
    while (true) {
        showMainMenu();
        std::string choice = getInput();
        
        if (choice == "1") {
            triggerAlert();
        } else if (choice == "2") {
            changeRecipientEmail();
        } else if (choice == "3" || choice == "q" || choice == "exit") {
            std::cout << ansiColor(33) << "Thank you for using the Home Security System. Goodbye!" << ansiReset() << std::endl;
            break;
        } else if (choice == "h" || choice == "help") {
            showHelpScreen();
        } else if (choice == "c" || choice == "clear") {
            std::cout << clearScreen();
        } else {
            std::cout << ansiColor(31) << "Invalid choice. Please try again." << ansiReset() << std::endl;
        }
    }
}
