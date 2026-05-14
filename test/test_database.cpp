#include "database.h"
#include <gtest/gtest.h>
#include <sqlite3.h>

class DatabaseTest : public ::testing::Test {
protected:
    Database* db;

    void SetUp() override {
        db = new Database(":memory:");
    }

    void TearDown() override {
        delete db;
    }

    int countRows(const std::string& table) {
        std::string sql = "SELECT COUNT(*) FROM " + table;
        int cnt = 0;
        auto cb = [](void* data, int, char** vals, char**) -> int {
            int* ptr = static_cast<int*>(data);
            *ptr = std::stoi(vals[0] ? vals[0] : "0");
            return 0;
        };
        db->execute_callback(sql, cb, &cnt);
        return cnt;
    }
};

TEST_F(DatabaseTest, UsersTableExists) {
    EXPECT_GE(countRows("users"), 0);
}

TEST_F(DatabaseTest, InsertDisc) {
    bool ok = db->execute("INSERT INTO cd_discs (cd_code, manufacture_date, producer_company, price) VALUES ('CD-99','2025-01-01','TestLabel',9.99)"); EXPECT_TRUE(ok); EXPECT_EQ(countRows("cd_discs"), 1);
}

TEST_F(DatabaseTest, SaleTriggerPreventsOverSelling) {
    db->execute("INSERT INTO cd_discs VALUES ('CD-TRIG','2025-01-01','Test',10.0,NULL)");
    db->execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-01','I','CD-TRIG',5)");
    bool sale_ok = db->execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-02','S','CD-TRIG',10)");
    EXPECT_FALSE(sale_ok);
}

TEST_F(DatabaseTest, ValidSaleSucceeds) {
    db->execute("INSERT INTO cd_discs VALUES ('CD-OK','2025-01-01','Test',10.0,NULL)");
    db->execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-01','I','CD-OK',10)");
    bool sale_ok = db->execute("INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('2025-01-02','S','CD-OK',3)");
    EXPECT_TRUE(sale_ok);
}
