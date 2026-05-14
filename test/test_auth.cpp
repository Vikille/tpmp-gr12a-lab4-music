#include <gtest/gtest.h>
#include <sqlite3.h>
#include <string>

#define private public
#include "auth.h"
#undef private

#include "database.h"

class AuthTest : public ::testing::Test {
protected:
    Database* db;
    AuthManager* auth;

    void SetUp() override {
        db = new Database(":memory:");
        db->execute("INSERT INTO users (username, password_hash, role) VALUES ('user1', 'user1', 'customer')");
        db->execute("INSERT INTO users (username, password_hash, role) VALUES ('admin1', 'admin1', 'admin')");
        auth = new AuthManager(*db);
    }

    void TearDown() override {
        delete auth;
        delete db;
    }

    void simulate_login(const std::string& user, const std::string& pass, 
                        bool expected, Role expected_role) {
        bool result = auth->check_credentials(user, pass);
        EXPECT_EQ(result, expected);
        if(result) {
            EXPECT_EQ(auth->get_role(), expected_role);
            EXPECT_EQ(auth->get_user(), user);
        }
    }
};

TEST_F(AuthTest, LoginValidUser) {
    simulate_login("user1", "user1", true, Role::Customer);
}

TEST_F(AuthTest, LoginInvalidPassword) {
    simulate_login("user1", "wrong", false, Role::None);
}

TEST_F(AuthTest, LoginNonExistentUser) {
    simulate_login("ghost", "ghost", false, Role::None);
}

TEST_F(AuthTest, LoginAdmin) {
    simulate_login("admin1", "admin1", true, Role::Admin);
}
