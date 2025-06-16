#pragma once
#include <string>
#include "nlohmann/json.hpp"

std::string get_application_support_path();
std::string get_config_file_path();
bool write_config_file(const std::string &path, nlohmann::json &config);
bool read_config_file(std::string &path, nlohmann::json &config);
bool validate_window_size(int width, int height);
void apply_window_size_defaults(int &width, int &height);