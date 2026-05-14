#include "database.h"
#include "auth.h"
#include "menu.h"
#include <iostream>

int main() {
    try {
        Database db("data/music_store.db");
        AuthManager auth(db);
        Menu menu(db, auth);
        menu.run();
    } catch(const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
