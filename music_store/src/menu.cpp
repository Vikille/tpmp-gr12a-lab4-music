#include "menu.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

Menu::Menu(Database& db, AuthManager& auth) : db_(db), auth_(auth) {}

void Menu::run() {
    if(!auth_.login()) {
        std::cout << "Выход.\n";
        return;
    }
    switch(auth_.get_role()) {
        case Role::Customer: customer_menu(); break;
        case Role::Manager:  manager_menu();  break;
        case Role::Admin:    admin_menu();    break;
        default: break;
    }
}

void Menu::customer_menu() {
    std::cout << "\n--- Меню покупателя ---\n";
    std::cout << "1. Просмотр всех дисков\n";
    std::cout << "2. Самый продаваемый диск\n";
    std::cout << "3. Самый популярный исполнитель\n";
    std::cout << "0. Выход\n";
    int choice = utils::input_int("Ваш выбор: ");
    while(choice != 0) {
        switch(choice) {
            case 1: view_discs(); break;
            case 2: view_bestseller(); break;
            case 3: view_top_performer(); break;
            default: std::cout << "Неверный пункт.\n";
        }
        std::cout << "\n--- Меню покупателя ---\n";
        std::cout << "1. Просмотр всех дисков\n";
        std::cout << "2. Самый продаваемый диск\n";
        std::cout << "3. Самый популярный исполнитель\n";
        std::cout << "0. Выход\n";
        choice = utils::input_int("Ваш выбор: ");
    }
}

void Menu::manager_menu() {
    std::cout << "\n--- Меню менеджера ---\n";
    std::cout << "1. Просмотр дисков (остатки)\n";
    std::cout << "2. Добавить поступление\n";
    std::cout << "3. Оформить продажу\n";
    std::cout << "4. Отчёт по авторам\n";
    std::cout << "5. Сформировать отчёт за период\n";
    std::cout << "0. Выход\n";
    int choice = utils::input_int("Ваш выбор: ");
    while(choice != 0) {
        switch(choice) {
            case 1: view_discs(); break;
            case 2: add_arrival(); break;
            case 3: add_sale(); break;
            case 4: report_by_author(); break;
            case 5: period_report(); break;
            default: std::cout << "Неверный пункт.\n";
        }
        std::cout << "\n--- Меню менеджера ---\n";
        std::cout << "1. Просмотр дисков (остатки)\n";
        std::cout << "2. Добавить поступление\n";
        std::cout << "3. Оформить продажу\n";
        std::cout << "4. Отчёт по авторам\n";
        std::cout << "5. Сформировать отчёт за период\n";
        std::cout << "0. Выход\n";
        choice = utils::input_int("Ваш выбор: ");
    }
}

void Menu::admin_menu() {
    std::cout << "\n--- Меню администратора ---\n";
    std::cout << "1. Добавить новый диск\n";
    std::cout << "2. Загрузить обложку\n";
    std::cout << "0. Выход\n";
    int choice = utils::input_int("Ваш выбор: ");
    while(choice != 0) {
        switch(choice) {
            case 1: admin_add_disc(); break;
            case 2: admin_upload_cover(); break;
            default: std::cout << "Неверный пункт.\n";
        }
        std::cout << "\n--- Меню администратора ---\n";
        std::cout << "1. Добавить новый диск\n";
        std::cout << "2. Загрузить обложку\n";
        std::cout << "0. Выход\n";
        choice = utils::input_int("Ваш выбор: ");
    }
}

// --- Реализация операций ---

void Menu::view_discs() {
    std::string sql = R"(
        SELECT d.cd_code, d.price,
               COALESCE(i.qty,0) AS incoming,
               COALESCE(s.qty,0) AS sold,
               COALESCE(i.qty,0) - COALESCE(s.qty,0) AS remaining
        FROM cd_discs d
        LEFT JOIN (SELECT cd_code, SUM(quantity) AS qty FROM operations WHERE operation_type='I' GROUP BY cd_code) i ON d.cd_code = i.cd_code
        LEFT JOIN (SELECT cd_code, SUM(quantity) AS qty FROM operations WHERE operation_type='S' GROUP BY cd_code) s ON d.cd_code = s.cd_code
        ORDER BY remaining DESC
    )";
    auto callback = [](void*, int cols, char** vals, char**) -> int {
        if(vals[0]) std::cout << std::left << std::setw(10) << vals[0]
                 << std::setw(8) << vals[1]
                 << std::setw(10) << vals[2]
                 << std::setw(8) << vals[3]
                 << std::setw(10) << vals[4] << "\n";
        return 0;
    };
    std::cout << std::left << std::setw(10) << "Код" << std::setw(8) << "Цена"
              << std::setw(10) << "Поступило" << std::setw(8) << "Продано"
              << std::setw(10) << "Остаток" << "\n";
    std::cout << std::string(46, '-') << "\n";
    db_.execute_callback(sql, callback);
}

void Menu::view_bestseller() {
    std::cout << "--- Самый продаваемый диск ---\n";
    db_.execute_callback("SELECT * FROM customer_bestseller",
        [](void*, int cols, char** vals, char** names) -> int {
            for(int i=0; i<cols; ++i) {
                if(names[i] && vals[i]) std::cout << names[i] << ": " << vals[i] << "\n";
            }
            std::cout << "------------------------\n";
            return 0;
        });
}

void Menu::view_top_performer() {
    std::cout << "--- Самый популярный исполнитель ---\n";
    db_.execute_callback("SELECT * FROM customer_top_performer",
        [](void*, int, char** vals, char**) -> int {
            std::cout << "Исполнитель: " << (vals[0]?vals[0]:"") << ", продано дисков: " << (vals[1]?vals[1]:"") << "\n";
            return 0;
        });
}

void Menu::add_arrival() {
    std::string code = utils::input_string("Код диска: ");
    int qty = utils::input_int("Количество: ", 1);
    std::string date = utils::get_current_date();
    std::string sql = "INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('"
                     + date + "','I','" + code + "'," + std::to_string(qty) + ")";
    if(db_.execute(sql)) std::cout << "Поступление добавлено.\n";
}

void Menu::add_sale() {
    std::string code = utils::input_string("Код диска: ");
    int qty = utils::input_int("Количество: ", 1);
    std::string date = utils::get_current_date();
    std::string sql = "INSERT INTO operations (operation_date, operation_type, cd_code, quantity) VALUES ('"
                     + date + "','S','" + code + "'," + std::to_string(qty) + ")";
    if(db_.execute(sql)) std::cout << "Продажа оформлена.\n";
    else std::cout << "Ошибка: возможно, недостаточно товара.\n";
}

void Menu::report_by_author() {
    std::string sql = R"(
        SELECT t.author, SUM(o.quantity) AS sold, SUM(o.quantity * d.price) AS revenue
        FROM operations o
        JOIN music_tracks t ON o.cd_code = t.cd_code
        JOIN cd_discs d ON o.cd_code = d.cd_code
        WHERE o.operation_type = 'S'
        GROUP BY t.author
        ORDER BY revenue DESC
    )";
    auto callback = [](void*, int, char** vals, char**) -> int {
        std::cout << "Автор: " << vals[0] << ", продано: " << vals[1] << ", выручка: " << vals[2] << "\n";
        return 0;
    };
    std::cout << "--- Отчёт по авторам ---\n";
    db_.execute_callback(sql, callback);
}

void Menu::period_report() {
    std::string start = utils::input_date("Начало периода (YYYY-MM-DD): ");
    std::string end = utils::input_date("Конец периода (YYYY-MM-DD): ");
    db_.execute("DELETE FROM cd_period_stats WHERE period_start='" + start + "' AND period_end='" + end + "'");
    std::string sql = "INSERT INTO cd_period_stats (cd_code, period_start, period_end, total_in, total_out) "
                     "SELECT cd_code, '" + start + "', '" + end + "', "
                     "COALESCE(SUM(CASE WHEN operation_type='I' THEN quantity ELSE 0 END),0), "
                     "COALESCE(SUM(CASE WHEN operation_type='S' THEN quantity ELSE 0 END),0) "
                     "FROM operations WHERE operation_date BETWEEN '" + start + "' AND '" + end + "' "
                     "GROUP BY cd_code";
    if(db_.execute(sql)) {
        std::cout << "Отчёт сформирован. Показать?\n";
        db_.execute_callback("SELECT * FROM cd_period_stats WHERE period_start='" + start + "'",
            [](void*, int, char** vals, char**) -> int {
                std::cout << "Диск: " << vals[0] << ", поступило: " << vals[3] << ", продано: " << vals[4] << "\n";
                return 0;
            });
    }
}

void Menu::admin_add_disc() {
    std::string code = utils::input_string("Код диска: ");
    std::string date = utils::input_date("Дата изготовления (YYYY-MM-DD): ");
    std::string company = utils::input_string("Производитель: ");
    double price = utils::input_int("Цена (целое число): ", 1); // можно заменить на double
    std::string sql = "INSERT INTO cd_discs (cd_code, manufacture_date, producer_company, price) VALUES ('"
                     + code + "','" + date + "','" + company + "'," + std::to_string(price) + ")";
    if(db_.execute(sql)) std::cout << "Диск добавлен.\n";
    else std::cout << "Ошибка добавления.\n";
}

void Menu::admin_upload_cover() {
    std::string code = utils::input_string("Код диска: ");
    std::string src_path = utils::input_string("Путь к файлу обложки: ");
    std::string dest = "data/covers/" + code + ".jpg";
    if(utils::copy_file(src_path, dest)) {
        db_.execute("UPDATE cd_discs SET cover_path='" + dest + "' WHERE cd_code='" + code + "'");
        std::cout << "Обложка загружена.\n";
    } else {
        std::cout << "Ошибка копирования файла.\n";
    }
}
