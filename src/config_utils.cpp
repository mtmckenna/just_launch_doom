#include <iostream>
#include <fstream>
#include <cassert>
#include <filesystem>
#include "nlohmann/json.hpp"

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <stdlib.h>
#else
#include <stdlib.h>
#endif

const std::string APP_NAME = "just_launch_doom";

std::string get_application_support_path()
{
#ifdef _WIN32 // Windows platform
    const char *appDataDir = getenv("APPDATA");
    assert(appDataDir != nullptr);

    std::string path = std::string(appDataDir) + "\\" + APP_NAME;
#elif __APPLE__ // macOS platform
    const char *homeDir = getenv("HOME");
    assert(homeDir != nullptr);

    std::string path = std::string(homeDir) + "/Library/Application Support/" + APP_NAME;
#else           // Other platforms (assuming Linux/Unix-like)
    const char *homeDir = getenv("HOME");
    assert(homeDir != nullptr);

    std::string path = std::string(homeDir) + "/." + APP_NAME;
#endif

    std::filesystem::create_directories(path);
    return path;
}

std::string get_config_file_path()
{
    return get_application_support_path() + "/config.json";
}

bool write_config_file(const std::string &path, nlohmann::json &config)
{
    // Create parent directory if it doesn't exist
    std::filesystem::path file_path(path);
    std::filesystem::path parent_dir = file_path.parent_path();
    if (!parent_dir.empty()) {
        std::filesystem::create_directories(parent_dir);
    }
    
    // Write to file
    std::ofstream file(path);
    if (file.is_open())
    {
        file << config.dump(4); // dump with 4 spaces indentation
        file.close();
        return true;
    }
    else
    {
        return false;
    }
}

bool read_config_file(std::string &path, nlohmann::json &config)
{
    // if file exists, load it and put data into config
    // if not, create it and write some default settings
    std::ifstream file(path);
    if (file.is_open())
    {
        try {
            file >> config;
            file.close();
            return true;
        } catch (const std::exception& e) {
            file.close();
            return false;
        }
    }
    else
    {
        // Create default config if file doesn't exist
        config = {
            {"resolution", {800, 600}},
            {"pwad_directories", nlohmann::json::array()},
            {"iwad_filepath", ""},
            {"iwads", nlohmann::json::array()},
            {"selected_iwad", ""},
            {"selected_pwads", nlohmann::json::array()},
            {"custom_params", ""},
            {"theme", "fire"},
            {"config_files", nlohmann::json::array()},
            {"selected_config", ""},
            {"font_size", 1.0f}
        };
        return write_config_file(path, config);
    }
}

bool validate_window_size(int width, int height)
{
    return (width >= 400 && height >= 300 && width <= 4096 && height <= 4096);
}

void apply_window_size_defaults(int &width, int &height)
{
    if (width < 400) width = 800;
    if (height < 300) height = 600;
}