#ifndef ALARM_SYSTEM_H
#define ALARM_SYSTEM_H

#include <map>
#include <string>
#include "config.h"

class AlarmSystem {
    private:
        Config config;
        std::string current_recipient_email;
        
        void showWelcomeScreen();
        void showMainMenu();
        void showHelpScreen();
        std::string getCurrentTimeString();
        bool sendAlertEmail(const std::string& subject, const std::string& body);
        
    public:
        AlarmSystem();
        void changeRecipientEmail();
        void triggerAlert();
        void run();
};

Config load_config();

#endif // ALARM_SYSTEM_H