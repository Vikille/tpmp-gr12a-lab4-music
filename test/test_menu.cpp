#include "menu.h"
#include "database.h"
#include "auth.h"
#include <gtest/gtest.h>
#include <sstream>

// Чтобы тестировать приватные методы, временно делаем их публичными
// через директиву препроцессора (только для тестов)
#define private public
#include "menu.h"
#undef private

TEST(MenuTest, ViewDiscsDoesNotCrash) {
    Database db(":memory:");
    db.execute("INSERT INTO cd_discs VALUES 
('CD-TEST','2025-01-01','TestLabel',9.99,NULL)");
    db.execute("INSERT INTO operations VALUES 
(NULL,'2025-01-01','I','CD-TEST',10)");

    AuthManager auth(db);
    Menu menu(db, auth);
    testing::internal::CaptureStdout();
    menu.view_discs();  // теперь публичный
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

TEST(MenuTest, AddArrivalIncreasesStock) {
    Database db(":memory:");
    db.execute("INSERT INTO cd_discs VALUES 
('CD-ARR','2025-01-01','Test',10.0,NULL)");
    AuthManager auth(db);
    Menu menu(db, auth);
    // Симулируем ввод
    std::istringstream fake_input("CD-ARR\n5\n");
    std::cin.rdbuf(fake_input.rdbuf());
    menu.add_arrival();
    // Проверим, что операция добавилась
    int cnt = 0;
    db.execute_callback("SELECT COUNT(*) FROM operations WHERE 
cd_code='CD-ARR' AND operation_type='I'",
        [](void* data, int, char** vals, char**) -> int {
            *static_cast<int*>(data) = std::stoi(vals[0]);
            return 0;
        }, &cnt);
    EXPECT_EQ(cnt, 1);
}
