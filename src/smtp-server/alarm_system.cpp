#include "alarm_system.h"
#include "smtp_client.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

// Function to load configuration from .env file
Config load_config() {
    Config config;
    config.port = 587; // Default SMTP port
    config.use_ssl = true; // Default to using SSL
    
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
                // Remove quotes if present
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

// AlarmSystem implementation
AlarmSystem::AlarmSystem() {
    // Initialize sensors with default mappings
    sensors['d'] = "Front Door";
    sensors['w'] = "Window";
    sensors['m'] = "Motion Detector";
    sensors['b'] = "Back Door";
    sensors['g'] = "Garage Door";
    sensors['p'] = "Patio Door";
    sensors['f'] = "Fire Alarm";
    sensors['c'] = "Carbon Monoxide";
    
    // Load email configuration
    config = load_config();
    
    // Set default recipient email from config
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
    std::cout << "║           ██╗  ██╗ ██████╗ ███╗   ███╗███████╗                        ║" << std::endl;
    std::cout << "║           ██║  ██║██╔═══██╗████╗ ████║██╔════╝                        ║" << std::endl;
    std::cout << "║           ███████║██║   ██║██╔████╔██║█████╗                          ║" << std::endl;
    std::cout << "║           ██╔══██║██║   ██║██║╚██╔╝██║██╔══╝                          ║" << std::endl;
    std::cout << "║           ██║  ██║╚██████╔╝██║ ╚═╝ ██║███████╗                        ║" << std::endl;
    std::cout << "║           ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝╚══════╝                        ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║         ███████╗███████╗ ██████╗██╗   ██╗██████╗ ██╗████████╗██╗   ██╗║" << std::endl;
    std::cout << "║         ██╔════╝██╔════╝██╔════╝██║   ██║██╔══██╗██║╚══██╔══╝╚██╗ ██╔╝║" << std::endl;
    std::cout << "║         ███████╗█████╗  ██║     ██║   ██║██████╔╝██║   ██║    ╚████╔╝ ║" << std::endl;
    std::cout << "║         ╚════██║██╔══╝  ██║     ██║   ██║██╔══██╗██║   ██║     ╚██╔╝  ║" << std::endl;
    std::cout << "║         ███████║███████╗╚██████╗╚██████╔╝██║  ██║██║   ██║      ██║   ║" << std::endl;
    std::cout << "║         ╚══════╝╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚═╝   ╚═╝      ╚═╝   ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "║              Welcome to the Home Security Alert System!                ║" << std::endl;
    std::cout << "║                                                                        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════════════════════╝" << ansiReset() << std::endl;
    std::cout << std::endl;
}

void AlarmSystem::showMainMenu() {
    std::cout << ansiColor(33) << "MAIN MENU:" << ansiReset() << std::endl;
    std::cout << "1. Monitoring Mode - Trigger sensors" << std::endl;
    std::cout << "2. Change Recipient Email" << std::endl;
    std::cout << "3. Send Test Alert" << std::endl;
    std::cout << "4. Exit" << std::endl;
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
    std::cout << "1. Monitoring Mode - Activate sensor monitoring where keyboard keys" << std::endl;
    std::cout << "   simulate different sensor triggers" << std::endl;
    std::cout << "2. Change Recipient Email - Update the email address that will receive alerts" << std::endl;
    std::cout << "3. Send Test Alert - Send a test email to verify your configuration" << std::endl;
    std::cout << "4. Exit - Close the application" << std::endl;
    std::cout << "h - Display this help screen" << std::endl;
    std::cout << "c - Clear the screen" << std::endl;
    std::cout << std::endl;
    
    std::cout << ansiColor(33) << "SENSOR KEYS IN MONITORING MODE:" << ansiReset() << std::endl;
    for (const auto& sensor : sensors) {
        std::cout << "'" << sensor.first << "' - Trigger " << sensor.second << " alarm" << std::endl;
    }
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

void AlarmSystem::displaySensors() {
    std::cout << "\n" << ansiColor(36) << "===== ALARM SYSTEM SENSORS =====" << ansiReset() << "\n";
    std::cout << "Current time: " << getCurrentTimeString() << "\n\n";
    
    int i = 0;
    for (const auto& sensor : sensors) {
        std::cout << ansiColor(32) << " [" << sensor.first << "] " << ansiReset();
        std::cout << sensor.second;
        
        // Create two columns
        if (++i % 2 == 0) {
            std::cout << std::endl;
        } else {
            std::cout << "\t\t";
        }
    }
    
    if (i % 2 != 0) {
        std::cout << std::endl;
    }
    
    std::cout << ansiColor(31) << " [q] " << ansiReset() << "Return to main menu\n";
    std::cout << ansiColor(36) << "==============================" << ansiReset() << "\n\n";
    std::cout << "System is " << ansiColor(32) << "ARMED" << ansiReset() << " and monitoring. Press any configured key to simulate a sensor trigger...\n" << std::endl;
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

void AlarmSystem::trigger_alarm(char sensor_key) {
    if (sensors.find(sensor_key) == sensors.end()) {
        std::cout << ansiColor(31) << "Unknown sensor: " << sensor_key << ansiReset() << std::endl;
        return;
    }
    
    std::string sensor_name = sensors[sensor_key];
    
    // Visual alarm feedback
    std::cout << "\n" << ansiColor(41) << "!!! ALARM TRIGGERED: " << sensor_name << " !!!" << ansiReset() << std::endl;
    
    // Flash the alarm message a few times for visual effect
    for (int i = 0; i < 3; i++) {
        std::cout << ansiColor(31) << "!!! ALERT !!! ALERT !!! ALERT !!!" << ansiReset() << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        std::cout << "\033[A\033[K"; // Move up one line and clear it
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
    
    // Get current time
    std::string current_time = getCurrentTimeString();
    
    // Create email content
    std::string subject = "SECURITY ALERT: " + sensor_name + " Triggered";
    std::string body = 
        "========== SECURITY ALERT NOTIFICATION ==========\r\n\r\n"
        "Time: " + current_time + "\r\n"
        "Sensor: " + sensor_name + "\r\n"
        "Status: TRIGGERED\r\n\r\n"
        "This is an automated message from your home security system. "
        "Please take appropriate action immediately.\r\n\r\n"
        "If this is a false alarm, you can reset the system remotely "
        "using the security app.\r\n\r\n"
        "===============================================\r\n"
        "DO NOT REPLY TO THIS EMAIL - THIS IS AN AUTOMATED MESSAGE";
    
    // Send email alert
    sendAlertEmail(subject, body);
}

void AlarmSystem::sendTestAlert() {
    std::cout << ansiColor(36) << "\nSending test alert email to " << current_recipient_email << "..." << ansiReset() << std::endl;
    
    std::string current_time = getCurrentTimeString();
    std::string subject = "Test Alert from Home Security System";
    std::string body = 
        "========== TEST ALERT ==========\r\n\r\n"
        "This is a TEST alert from your home security system.\r\n"
        "Time: " + current_time + "\r\n\r\n"
        "If you received this email, your security system is properly configured "
        "and can send alerts in case of an emergency.\r\n\r\n"
        "==============================\r\n"
        "Home Security System";
    
    if (sendAlertEmail(subject, body)) {
        std::cout << ansiColor(32) << "Test alert sent successfully!" << ansiReset() << std::endl;
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
    
    // Properly disconnect from the server
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
            // Monitoring mode
            std::cout << clearScreen();
            displaySensors();
            
            while (true) {
                char key = getch_nonblock();
                if (key != 0) {
                    if (key == 'q') {
                        std::cout << ansiColor(33) << "Exiting monitoring mode..." << ansiReset() << std::endl;
                        break;
                    } else if (sensors.find(key) != sensors.end()) {
                        trigger_alarm(key);
                        displaySensors(); // Refresh display after alarm
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Add small delay to reduce CPU usage
            }
            
            std::cout << clearScreen();
        }
        else if (choice == "2") {
            // Change recipient email
            changeRecipientEmail();
        }
        else if (choice == "3") {
            // Send test alert
            sendTestAlert();
        }
        else if (choice == "4" || choice == "q" || choice == "exit") {
            // Exit the program
            std::cout << ansiColor(33) << "Thank you for using the Home Security System. Goodbye!" << ansiReset() << std::endl;
            break;
        }
        else if (choice == "h" || choice == "help") {
            // Show help screen
            showHelpScreen();
        }
        else if (choice == "c" || choice == "clear") {
            // Clear screen
            std::cout << clearScreen();
        }
        else {
            std::cout << ansiColor(31) << "Invalid choice. Please try again." << ansiReset() << std::endl;
        }
    }
}
