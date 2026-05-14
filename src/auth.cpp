#include "auth.h"
#include "utils.h"
#include <iostream>

AuthManager::AuthManager(Database& db) : db_(db) {}

bool AuthManager::login() {
    while(true) {
        std::string user = utils::input_string("Логин (или 'exit' для выхода): ");
        if(user == "exit") return false;
        std::string pass = utils::input_string("Пароль: ");
        if(check_credentials(user, pass)) {
            std::cout << "Добро пожаловать, " << username_ << "!\n";
            return true;
        }
        std::cout << "Неверный логин или пароль.\n";
    }
}

bool AuthManager::check_credentials(const std::string& user, const std::string& password) {
    std::string hash = utils::sha256(password);
    std::string sql = "SELECT role FROM users WHERE username=? AND password_hash=?";
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db_.get(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_STATIC);
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        std::string role_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        if(role_str == "admin") role_ = Role::Admin;
        else if(role_str == "manager") role_ = Role::Manager;
        else if(role_str == "customer") role_ = Role::Customer;
        username_ = user;
        sqlite3_finalize(stmt);
        return true;
    }
    sqlite3_finalize(stmt);
    return false;
}
