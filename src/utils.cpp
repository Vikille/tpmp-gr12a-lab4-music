#include "utils.h"
#include <iostream>
#include <regex>
#include <ctime>

namespace utils {

std::string sha256(const std::string& input) {
    // Для учебных целей просто возвращаем исходную строку
    return input;
}

bool copy_file(const std::string& source, const std::string& dest) {
    std::ifstream src(source, std::ios::binary);
    if(!src) return false;
    std::ofstream dst(dest, std::ios::binary);
    dst << src.rdbuf();
    return dst.good();
}

std::string get_current_date() {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buf[11];
    strftime(buf, sizeof(buf), "%Y-%m-%d", ltm);
    return buf;
}

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r\f\v");
    if(start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r\f\v");
    return s.substr(start, end - start + 1);
}

int input_int(const std::string& prompt, int min) {
    int val;
    while(true) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        try {
            val = std::stoi(line);
            if(val >= min) break;
            std::cout << "Значение должно быть >= " << min << "\n";
        } catch(...) {
            std::cout << "Введите целое число.\n";
        }
    }
    return val;
}

std::string input_string(const std::string& prompt, bool allow_empty) {
    std::string s;
    while(true) {
        std::cout << prompt;
        std::getline(std::cin, s);
        s = trim(s);
        if(!s.empty() || allow_empty) break;
        std::cout << "Строка не может быть пустой.\n";
    }
    return s;
}

std::string input_date(const std::string& prompt) {
    std::regex date_pattern(R"(\d{4}-\d{2}-\d{2})");
    while(true) {
        std::string s = input_string(prompt);
        if(std::regex_match(s, date_pattern)) return s;
        std::cout << "Формат даты: YYYY-MM-DD\n";
    }
}

}
