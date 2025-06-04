#ifndef EMAIL_MANAGER_H
#define EMAIL_MANAGER_H

#include <vector>
#include <string>
#include "config.h"
#include "pop3_client.h"

class EmailManager {
    private:
        Config config;
        std::vector<EmailInfo> email_list;
        int current_selection;
        bool connected;
        
        void showWelcomeScreen();
        void showMainMenu();
        void showHelpScreen();
        void connectToServer();
        void refreshEmailList();
        void showEmailList();
        void toggleEmailSelection(int index);
        void deleteSelectedEmails();
        void viewLatestEmail();
        
    public:
        EmailManager();
        void run();
};

#endif // EMAIL_MANAGER_H
