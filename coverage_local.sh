#!/bin/bash
echo ">>> Локальный запуск тестов и покрытия"
# Проверка зависимостей
if ! command -v g++ &> /dev/null; then
    echo "g++ не найден. Установите: brew install gcc"
    exit 1
fi
if ! command -v lcov &> /dev/null; then
    echo "lcov не найден. Установите: brew install lcov"
    exit 1
fi

g++ -std=c++17 --coverage \
    -I includes -I src \
    test/test_runner.cpp \
    src/utils.cpp src/auth.cpp src/database.cpp src/menu.cpp \
    -lsqlite3 \
    -o test_runner

./test_runner
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/test/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report
echo "Отчёт создан в coverage_report/index.html"
