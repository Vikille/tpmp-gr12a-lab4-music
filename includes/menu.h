#pragma once
#include "database.h"
#include "auth.h"

class Menu {
public:
    Menu(Database& db, AuthManager& auth);
    void run();
private:
    Database& db_;
    AuthManager& auth_;
    void customer_menu();
    void manager_menu();
    void admin_menu();
    void view_discs();
    void view_bestseller();
    void view_top_performer();
    void add_arrival();
    void add_sale();
    void report_by_author();
    void period_report();
    void admin_add_disc();
    void admin_upload_cover();
};
