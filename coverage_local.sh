#!/bin/bash
echo ">>> Локальный запуск тестов и генерация отчёта о покрытии (требуется g++ и lcov)"

# Проверка зависимостей
if ! command -v g++ &> /dev/null; then
    echo "g++ не найден. Установите: brew install gcc (macOS) или apt install g++ (Linux)"
    exit 1
fi
if ! command -v lcov &> /dev/null; then
    echo "lcov не найден. Установите: brew install lcov (macOS) или apt install lcov (Linux)"
    exit 1
fi

# Компиляция тестов с флагами покрытия
g++ -std=c++17 --coverage \
    -I includes -I src \
    test/test_utils.cpp test/test_auth.cpp test/test_database.cpp test/test_menu.cpp \
    src/utils.cpp src/auth.cpp src/database.cpp src/menu.cpp \
    -lgtest -lgtest_main -lsqlite3 \
    -o test_runner

# Запуск тестов
./test_runner

# Генерация отчёта
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/gtest/*' '*/test/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report
echo "Открыть: open coverage_report/index.html  (или xdg-open)"
