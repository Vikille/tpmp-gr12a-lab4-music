#include "database.h"
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

Database::Database(const std::string& path) : path_(path) {
    if(sqlite3_open(path_.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("Не могу открыть БД: " + std::string(sqlite3_errmsg(db_)));
    }
    execute("PRAGMA foreign_keys = ON;");
    create_schema();
    seed_test_data();
}

Database::~Database() {
    sqlite3_close(db_);
}

bool Database::execute(const std::string& sql) {
    char* errmsg = nullptr;
    if(sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "SQL ошибка: %s\n", errmsg);
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

bool Database::execute_callback(const std::string& sql,
                               int (*callback)(void*,int,char**,char**),
                               void* data) {
    char* errmsg = nullptr;
    if(sqlite3_exec(db_, sql.c_str(), callback, data, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "SQL ошибка: %s\n", errmsg);
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

void Database::create_schema() {
    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            role TEXT NOT NULL CHECK(role IN ('customer','manager','admin'))
        );
        CREATE TABLE IF NOT EXISTS cd_discs (
            cd_code TEXT PRIMARY KEY,
            manufacture_date DATE NOT NULL,
            producer_company TEXT NOT NULL,
            price REAL NOT NULL CHECK(price > 0),
            cover_path TEXT
        );
        CREATE TABLE IF NOT EXISTS music_tracks (
            track_id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            author TEXT NOT NULL,
            performer TEXT NOT NULL,
            cd_code TEXT NOT NULL,
            FOREIGN KEY (cd_code) REFERENCES cd_discs(cd_code) ON DELETE RESTRICT
        );
        CREATE TABLE IF NOT EXISTS operations (
            op_id INTEGER PRIMARY KEY AUTOINCREMENT,
            operation_date DATE NOT NULL,
            operation_type TEXT NOT NULL CHECK(operation_type IN ('I','S')),
            cd_code TEXT NOT NULL,
            quantity INTEGER NOT NULL CHECK(quantity > 0),
            FOREIGN KEY (cd_code) REFERENCES cd_discs(cd_code) ON DELETE RESTRICT
        );
        CREATE TABLE IF NOT EXISTS cd_period_stats (
            cd_code TEXT NOT NULL,
            period_start DATE NOT NULL,
            period_end DATE NOT NULL,
            total_in INTEGER NOT NULL DEFAULT 0,
            total_out INTEGER NOT NULL DEFAULT 0,
            PRIMARY KEY (cd_code, period_start, period_end)
        );
        CREATE TRIGGER IF NOT EXISTS check_sale_stock
        BEFORE INSERT ON operations
        FOR EACH ROW
        WHEN NEW.operation_type = 'S'
        BEGIN
            SELECT RAISE(ABORT, 'Недостаточно товара на складе')
            WHERE (SELECT COALESCE(SUM(quantity),0) FROM operations WHERE cd_code = NEW.cd_code AND operation_type='I')
                - (SELECT COALESCE(SUM(quantity),0) FROM operations WHERE cd_code = NEW.cd_code AND operation_type='S')
                < NEW.quantity;
        END;
        CREATE VIEW IF NOT EXISTS customer_bestseller AS
        WITH sales AS (SELECT cd_code, SUM(quantity) AS qty FROM operations WHERE operation_type='S' GROUP BY cd_code)
        SELECT d.*, t.title, t.author, t.performer
        FROM cd_discs d
        JOIN (SELECT cd_code FROM sales WHERE qty = (SELECT MAX(qty) FROM sales) LIMIT 1) bs ON d.cd_code = bs.cd_code
        LEFT JOIN music_tracks t ON d.cd_code = t.cd_code;
        CREATE VIEW IF NOT EXISTS customer_top_performer AS
        WITH perf_sales AS (
            SELECT t.performer, SUM(o.quantity) AS total
            FROM operations o JOIN music_tracks t ON o.cd_code = t.cd_code
            WHERE o.operation_type='S'
            GROUP BY t.performer
        )
        SELECT performer, total FROM perf_sales WHERE total = (SELECT MAX(total) FROM perf_sales);
    )";
    execute(schema);
}

void Database::seed_test_data() {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM users", -1, &stmt, nullptr);
    int count = 0;
    if(sqlite3_step(stmt) == SQLITE_ROW) count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if(count > 0) return;

    // Пароли хранятся открытым текстом (учебный вариант)
    execute("INSERT INTO users (username, password_hash, role) VALUES ('admin', 'admin', 'admin');");
    execute("INSERT INTO users (username, password_hash, role) VALUES ('manager', 'manager', 'manager');");
    execute("INSERT INTO users (username, password_hash, role) VALUES ('customer', 'customer', 'customer');");

    execute("INSERT INTO cd_discs VALUES ('CD-001','2025-01-15','Melody Inc.',12.99,NULL)");
    execute("INSERT INTO cd_discs VALUES ('CD-002','2025-02-10','SoundWave',9.99,NULL)");
    execute("INSERT INTO cd_discs VALUES ('CD-003','2025-03-05','Vinyl Records',15.50,NULL)");
    execute("INSERT INTO music_tracks (title,author,performer,cd_code) VALUES ('Moonlight','Smith','John Band','CD-001')");
    execute("INSERT INTO music_tracks (title,author,performer,cd_code) VALUES ('Sunrise','Smith','John Band','CD-001')");
    execute("INSERT INTO music_tracks (title,author,performer,cd_code) VALUES ('Storm','Black','Rockers','CD-002')");
    execute("INSERT INTO music_tracks (title,author,performer,cd_code) VALUES ('Calm','Black','Rockers','CD-002')");
    execute("INSERT INTO music_tracks (title,author,performer,cd_code) VALUES ('Ocean','White','Elena Duo','CD-003')");
    execute("INSERT INTO operations VALUES (NULL,'2025-02-01','I','CD-001',100)");
    execute("INSERT INTO operations VALUES (NULL,'2025-02-15','I','CD-002',80)");
    execute("INSERT INTO operations VALUES (NULL,'2025-03-01','I','CD-003',50)");
    execute("INSERT INTO operations VALUES (NULL,'2025-02-10','S','CD-001',5)");
    execute("INSERT INTO operations VALUES (NULL,'2025-02-20','S','CD-001',10)");
    execute("INSERT INTO operations VALUES (NULL,'2025-02-28','S','CD-002',20)");
    execute("INSERT INTO operations VALUES (NULL,'2025-03-05','S','CD-003',5)");
}
