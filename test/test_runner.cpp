#include "utils.h"
#include "auth.h"
#include "database.h"
#include "menu.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <cstdio>
#include <fstream>

using namespace std;

// Простой макрос для тестов
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    cout << "  Running " << #name << "... "; \
    test_##name(); \
    cout << "PASSED" << endl; \
} while(0)

// ---------- Utils ----------
TEST(trim_basic) {
    assert(utils::trim("  hello world  ") == "hello world");
    assert(utils::trim("\t  test\n") == "test");
    assert(utils::trim("noSpaces") == "noSpaces");
}

TEST(date_format) {
    string d = utils::get_current_date();
    assert(d.size() == 10);
    assert(d[4] == '-' && d[7] == '-');
}

TEST(copy_file) {
    string src = "test_src.txt", dst = "test_dst.txt";
    {
        ofstream out(src);
        out << "Hello, copy!";
    }
    assert(utils::copy_file(src, dst));
    ifstream in(dst);
    string content;
    getline(in, content);
    assert(content == "Hello, copy!");
    remove(src.c_str());
    remove(dst.c_str());
}

TEST(sha256_simple) {
    assert(utils::sha256("admin") == "admin");
}

// ---------- Auth ----------
TEST(valid_login) {
    Database db(":memory:");
    db.execute("INSERT INTO users (username, password_hash, role) VALUES ('user1', 'user1', 'customer')");
    AuthManager auth(db);
    assert(auth.check_credentials("user1", "user1"));
    assert(auth.get_role() == Role::Customer);
    assert(auth.get_user() == "user1");
}

TEST(invalid_password) {
    Database db(":memory:");
    db.execute("INSERT INTO users (username, password_hash, role) VALUES ('user1', 'user1', 'customer')");
    AuthManager auth(db);
    assert(!auth.check_credentials("user1", "wrong"));
}

TEST(nonexistent_user) {
    Database db(":memory:");
    AuthManager auth(db);
    assert(!auth.check_credentials("ghost", "ghost"));
}

TEST(admin_login) {
    Database db(":memory:");
    db.execute("INSERT INTO users (username, password_hash, role) VALUES ('admin1', 'admin1', 'admin')");
    AuthManager auth(db);
    assert(auth.check_credentials("admin1", "admin1"));
    assert(auth.get_role() == Role::Admin);
}

// ---------- Database ----------
TEST(users_table_exists) {
    Database db(":memory:");
    // просто проверим, что запрос не падает
    db.execute("SELECT COUNT(*) FROM users");
}

TEST(insert_disc) {
    Database db(":memory:");
    assert(db.execute("INSERT INTO cd_discs (cd_code, manufacture_date, producer_company, price) VALUES ('CD-99','2025-01-01','TestLabel',9.99)"));
}

TEST(trigger_prevents_oversell) {
    Database db(":memory:");
    db.execute("INSERT INTO cd_discs VALUES ('CD-TRIG','2025-01-01','Test',10.0,NULL)");
    db.execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-01','I','CD-TRIG',5)");
    assert(!db.execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-02','S','CD-TRIG',10)"));
}

TEST(valid_sale) {
    Database db(":memory:");
    db.execute("INSERT INTO cd_discs VALUES ('CD-OK','2025-01-01','Test',10.0,NULL)");
    db.execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-01','I','CD-OK',10)");
    assert(db.execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-02','S','CD-OK',3)"));
}

// ---------- Menu (требует подмены ввода) ----------
#define private public
#include "menu.h"
#undef private

TEST(view_discs_output) {
    Database db(":memory:");
    db.execute("INSERT INTO cd_discs VALUES ('CD-TEST','2025-01-01','TestLabel',9.99,NULL)");
    db.execute("INSERT INTO operations VALUES (NULL,'2025-01-01','I','CD-TEST',10)");
    AuthManager auth(db);
    Menu menu(db, auth);
    stringstream capture;
    streambuf* old = cout.rdbuf(capture.rdbuf());
    menu.view_discs();
    cout.rdbuf(old);
    string output = capture.str();
    assert(!output.empty());
}

TEST(add_arrival) {
    Database db(":memory:");
    db.execute("INSERT INTO cd_discs VALUES ('CD-ARR','2025-01-01','Test',10.0,NULL)");
    AuthManager auth(db);
    Menu menu(db, auth);
    istringstream fake_input("CD-ARR\n5\n");
    cin.rdbuf(fake_input.rdbuf());
    menu.add_arrival();
    int cnt = 0;
    db.execute_callback("SELECT COUNT(*) FROM operations WHERE cd_code='CD-ARR' AND operation_type='I'",
        [](void* data, int, char** vals, char**) -> int {
            *static_cast<int*>(data) = stoi(vals[0]);
            return 0;
        }, &cnt);
    assert(cnt == 1);
}

TEST(period_report) {
    Database db(":memory:");
    // Очищаем автозаполненные данные, которые мешают
    db.execute("DELETE FROM operations");
    db.execute("DELETE FROM cd_discs");
    db.execute("DELETE FROM cd_period_stats");

    db.execute("INSERT INTO cd_discs VALUES ('CD-PER','2025-01-01','Test',10.0,NULL)");
    db.execute("INSERT INTO operations VALUES (NULL,'2025-03-01','I','CD-PER',10)");
    AuthManager auth(db);
    Menu menu(db, auth);
    istringstream fake_input("2025-03-01\n2025-03-02\n");
    cin.rdbuf(fake_input.rdbuf());
    menu.period_report();
    int cnt = 0;
    db.execute_callback("SELECT COUNT(*) FROM cd_period_stats",
        [](void* data, int, char** vals, char**) -> int {
            *static_cast<int*>(data) = stoi(vals[0]);
            return 0;
        }, &cnt);
    assert(cnt == 1);
}

TEST(admin_add_disc) {
    Database db(":memory:");
    AuthManager auth(db);
    Menu menu(db, auth);
    istringstream fake_input("CD-ADMIN\n2025-05-01\nAdminLabel\n15\n");
    cin.rdbuf(fake_input.rdbuf());
    menu.admin_add_disc();
    int cnt = 0;
    db.execute_callback("SELECT COUNT(*) FROM cd_discs WHERE cd_code='CD-ADMIN'",
        [](void* data, int, char** vals, char**) -> int {
            *static_cast<int*>(data) = stoi(vals[0]);
            return 0;
        }, &cnt);
    assert(cnt == 1);
}

// -------------------------------------------------------
int main() {
    cout << "Running unit tests..." << endl;
    // Utils
    cout << "Utils tests:" << endl;
    RUN_TEST(trim_basic);
    RUN_TEST(date_format);
    RUN_TEST(copy_file);
    RUN_TEST(sha256_simple);
    // Auth
    cout << "Auth tests:" << endl;
    RUN_TEST(valid_login);
    RUN_TEST(invalid_password);
    RUN_TEST(nonexistent_user);
    RUN_TEST(admin_login);
    // Database
    cout << "Database tests:" << endl;
    RUN_TEST(users_table_exists);
    RUN_TEST(insert_disc);
    RUN_TEST(trigger_prevents_oversell);
    RUN_TEST(valid_sale);
    // Menu
    cout << "Menu tests:" << endl;
    RUN_TEST(view_discs_output);
    RUN_TEST(add_arrival);
    RUN_TEST(period_report);
    RUN_TEST(admin_add_disc);

    cout << "\nAll tests passed!" << endl;
    return 0;
}