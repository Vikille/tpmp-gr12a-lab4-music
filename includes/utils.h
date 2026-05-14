#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <fstream>

namespace utils {
    std::string sha256(const std::string& input);
    bool copy_file(const std::string& source, const std::string& dest);
    std::string get_current_date();
    std::string trim(const std::string& s);
    int input_int(const std::string& prompt, int min = 0);
    std::string input_string(const std::string& prompt, bool allow_empty = false);
    std::string input_date(const std::string& prompt);
}
