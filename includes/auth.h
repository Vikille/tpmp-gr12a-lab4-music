#pragma once
#include "database.h"
#include <string>

enum class Role { None, Customer, Manager, Admin };

class AuthManager {
public:
    AuthManager(Database& db);
    bool login();
    Role get_role() const { return role_; }
    std::string get_user() const { return username_; }
private:
    Database& db_;
    Role role_ = Role::None;
    std::string username_;
    bool check_credentials(const std::string& user, const std::string& password);
};
