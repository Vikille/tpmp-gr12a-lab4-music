#pragma once
#include <sqlite3.h>
#include <string>
#include <functional>
#include <vector>
#include <stdexcept>

class Database {
public:
    Database(const std::string& path);
    ~Database();
    sqlite3* get() const { return db_; }
    bool execute(const std::string& sql);
    bool execute_callback(const std::string& sql,
                         int (*callback)(void*,int,char**,char**),
                         void* data = nullptr);
    void create_schema();
    void seed_test_data();
private:
    sqlite3* db_ = nullptr;
    std::string path_;
};
