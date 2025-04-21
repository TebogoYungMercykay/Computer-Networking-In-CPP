#ifndef ALARM_SYSTEM_H
#define ALARM_SYSTEM_H

#include <map>
#include <string>
#include "config.h"

class AlarmSystem {
    private:
        std::map<char, std::string> sensors;
        Config config;
        std::string current_recipient_email;
        
        void showWelcomeScreen();
        void showMainMenu();
        void showHelpScreen();
        std::string getCurrentTimeString();
        bool sendAlertEmail(const std::string& subject, const std::string& body);
        
    public:
        AlarmSystem();
        void displaySensors();
        void changeRecipientEmail();
        void trigger_alarm(char sensor_key);
        void sendTestAlert();
        void run();
};

// Function to load configuration from .env file
Config load_config();

#endif // ALARM_SYSTEM_H