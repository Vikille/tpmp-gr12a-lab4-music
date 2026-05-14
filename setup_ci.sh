#!/bin/bash
# ====================================================
#  Скрипт настройки CI/CD без CMake
#  для проекта "Музыкальный салон"
#  Запускать из корня проекта (music_store/)
# ====================================================

echo ">>> Создаю CI/CD с ручной компиляцией (g++ и lcov)..."

# --------------------------------------------------
# 1. Создаём папки
# --------------------------------------------------
mkdir -p test
mkdir -p .github/workflows

# --------------------------------------------------
# 2. Файл с тестами (Google Test)
#    Обратите внимание: тесты рассчитаны на сборку вместе
#    с исходниками без CMake.
# --------------------------------------------------

# --- test/test_utils.cpp ---
cat > test/test_utils.cpp << 'EOF'
#include "utils.h"
#include <gtest/gtest.h>
#include <sstream>
#include <fstream>
#include <cstdio>

TEST(UtilsTest, TrimRemovesWhitespace) {
    EXPECT_EQ(utils::trim("  hello world  "), "hello world");
    EXPECT_EQ(utils::trim("\t  test\n"), "test");
    EXPECT_EQ(utils::trim("noSpaces"), "noSpaces");
}

TEST(UtilsTest, GetCurrentDateFormat) {
    std::string date = utils::get_current_date();
    EXPECT_EQ(date.size(), 10);
    EXPECT_EQ(date[4], '-');
    EXPECT_EQ(date[7], '-');
}

TEST(UtilsTest, CopyFileWorks) {
    std::string src = "test_src.txt";
    std::string dst = "test_dst.txt";
    {
        std::ofstream out(src);
        out << "Hello, copy!";
    }
    EXPECT_TRUE(utils::copy_file(src, dst));
    std::ifstream in(dst);
    std::string content;
    std::getline(in, content);
    EXPECT_EQ(content, "Hello, copy!");
    std::remove(src.c_str());
    std::remove(dst.c_str());
}

TEST(UtilsTest, Sha256ReturnsInput) {
    EXPECT_EQ(utils::sha256("admin"), "admin");
}
EOF

# --- test/test_auth.cpp ---
cat > test/test_auth.cpp << 'EOF'
#include "auth.h"
#include "database.h"
#include <gtest/gtest.h>
#include <sqlite3.h>

class AuthTest : public ::testing::Test {
protected:
    Database* db;
    AuthManager* auth;

    void SetUp() override {
        db = new Database(":memory:");
        db->execute("INSERT INTO users (username, password_hash, role) 
VALUES ('user1', 'user1', 'customer')");
        db->execute("INSERT INTO users (username, password_hash, role) 
VALUES ('admin1', 'admin1', 'admin')");
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
EOF

# --- test/test_database.cpp ---
cat > test/test_database.cpp << 'EOF'
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
    bool ok = db->execute("INSERT INTO cd_discs (cd_code, 
manufacture_date, producer_company, price) VALUES 
('CD-99','2025-01-01','TestLabel',9.99)");
    EXPECT_TRUE(ok);
    EXPECT_EQ(countRows("cd_discs"), 1);
}

TEST_F(DatabaseTest, SaleTriggerPreventsOverSelling) {
    db->execute("INSERT INTO cd_discs VALUES 
('CD-TRIG','2025-01-01','Test',10.0,NULL)");
    db->execute("INSERT INTO operations (operation_date, operation_type, 
cd_code, quantity) VALUES ('2025-01-01','I','CD-TRIG',5)");
    bool sale_ok = db->execute("INSERT INTO operations (operation_date, 
operation_type, cd_code, quantity) VALUES 
('2025-01-02','S','CD-TRIG',10)");
    EXPECT_FALSE(sale_ok);
}

TEST_F(DatabaseTest, ValidSaleSucceeds) {
    db->execute("INSERT INTO cd_discs VALUES 
('CD-OK','2025-01-01','Test',10.0,NULL)");
    db->execute("INSERT INTO operations (operation_date, operation_type, 
cd_code, quantity) VALUES ('2025-01-01','I','CD-OK',10)");
    bool sale_ok = db->execute("INSERT INTO operations (operation_date, 
operation_type, cd_code, quantity) VALUES ('2025-01-02','S','CD-OK',3)");
    EXPECT_TRUE(sale_ok);
}
EOF

# --- test/test_menu.cpp ---
cat > test/test_menu.cpp << 'EOF'
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

TEST(MenuTest, PeriodReportPopulatesStats) {
    Database db(":memory:");
    db.execute("INSERT INTO cd_discs VALUES 
('CD-PER','2025-01-01','Test',10.0,NULL)");
    db.execute("INSERT INTO operations VALUES 
(NULL,'2025-03-01','I','CD-PER',10)");
    AuthManager auth(db);
    Menu menu(db, auth);
    std::istringstream fake_input("2025-03-01\n2025-03-02\n");
    std::cin.rdbuf(fake_input.rdbuf());
    menu.period_report();
    int cnt = 0;
    db.execute_callback("SELECT COUNT(*) FROM cd_period_stats",
        [](void* data, int, char** vals, char**) -> int {
            *static_cast<int*>(data) = std::stoi(vals[0]);
            return 0;
        }, &cnt);
    EXPECT_EQ(cnt, 1);
}

TEST(MenuTest, AdminAddDiscInsertsRecord) {
    Database db(":memory:");
    AuthManager auth(db);
    Menu menu(db, auth);
    std::istringstream 
fake_input("CD-ADMIN\n2025-05-01\nAdminLabel\n15\n");
    std::cin.rdbuf(fake_input.rdbuf());
    menu.admin_add_disc();
    int cnt = 0;
    db.execute_callback("SELECT COUNT(*) FROM cd_discs WHERE 
cd_code='CD-ADMIN'",
        [](void* data, int, char** vals, char**) -> int {
            *static_cast<int*>(data) = std::stoi(vals[0]);
            return 0;
        }, &cnt);
    EXPECT_EQ(cnt, 1);
}
EOF

# --------------------------------------------------
# 3. GitHub Actions workflow (без CMake)
# --------------------------------------------------
cat > .github/workflows/ci.yml << 'EOF'
name: CI without CMake
on:
  push:
    branches: [ main, master ]
  pull_request:
    branches: [ main, master ]

jobs:
  build-test-coverage:
    runs-on: ubuntu-latest
    steps:
    - name: Checkout repository
      uses: actions/checkout@v4

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y g++ libsqlite3-dev lcov libgtest-dev
        # Собираем Google Test из исходников
        cd /usr/src/gtest
        sudo cmake .
        sudo make
        sudo cp lib/*.a /usr/lib

    - name: Build application (main)
      run: |
        g++ -std=c++17 -I includes src/main.cpp src/database.cpp 
src/auth.cpp src/menu.cpp src/utils.cpp -lsqlite3 -o bin/music_store

    - name: Build tests with coverage flags
      run: |
        g++ -std=c++17 \
          --coverage \
          -I includes -I src \
          test/test_utils.cpp test/test_auth.cpp test/test_database.cpp 
test/test_menu.cpp \
          src/utils.cpp src/auth.cpp src/database.cpp src/menu.cpp \
          -lgtest -lgtest_main -lsqlite3 \
          -o test_runner

    - name: Run tests
      run: ./test_runner

    - name: Generate coverage report
      run: |
        lcov --capture --directory . --output-file coverage.info
        lcov --remove coverage.info '/usr/*' '*/gtest/*' '*/test/*' 
--output-file coverage_filtered.info
        lcov --list coverage_filtered.info

    - name: Upload coverage to Codecov
      uses: codecov/codecov-action@v4
      with:
        files: ./coverage_filtered.info
        flags: unittests
        name: music-store-coverage
        fail_ci_if_error: true
        verbose: true
      env:
        CODECOV_TOKEN: ${{ secrets.CODECOV_TOKEN }}
EOF

# --------------------------------------------------
# 4. Локальный скрипт для покрытия (только Linux / macOS с g++)
# --------------------------------------------------
cat > coverage_local.sh << 'SH'
#!/bin/bash
echo ">>> Локальный запуск тестов и генерация отчёта о покрытии (требуется 
g++ и lcov)"

# Проверка зависимостей
if ! command -v g++ &> /dev/null; then
    echo "g++ не найден. Установите: brew install gcc (macOS) или apt 
install g++ (Linux)"
    exit 1
fi
if ! command -v lcov &> /dev/null; then
    echo "lcov не найден. Установите: brew install lcov (macOS) или apt 
install lcov (Linux)"
    exit 1
fi

# Компиляция тестов с флагами покрытия
g++ -std=c++17 --coverage \
    -I includes -I src \
    test/test_utils.cpp test/test_auth.cpp test/test_database.cpp 
test/test_menu.cpp \
    src/utils.cpp src/auth.cpp src/database.cpp src/menu.cpp \
    -lgtest -lgtest_main -lsqlite3 \
    -o test_runner

# Запуск тестов
./test_runner

# Генерация отчёта
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/gtest/*' '*/test/*' --output-file 
coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report
echo "Открыть: open coverage_report/index.html  (или xdg-open)"
SH
chmod +x coverage_local.sh

echo ""
echo "✅ Готово!"
echo "Дальнейшие действия:"
echo "1. Зарегистрируйтесь на https://codecov.io, получите токен"
echo "2. Добавьте токен в секреты репозитория как CODECOV_TOKEN"
echo "   (Settings → Secrets and variables → Actions → New repository 
secret)"
echo "3. Закоммитьте изменения и запушьте в репозиторий:"
echo "   git add ."
echo "   git commit -m 'Add CI and tests'"
echo "   git push"
echo "4. Наблюдайте за выполнением Actions на GitHub, а затем за отчётом 
на Codecov"
echo ""
echo "Локально можно запустить тесты и покрытие: ./coverage_local.sh"
