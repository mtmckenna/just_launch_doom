#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/nlohmann/json.hpp"
#include "../src/config_utils.h"
#include <filesystem>
#include <fstream>
#include <cstdio>

// Include the config migration function from config_migration.cpp
nlohmann::json migrate_config(nlohmann::json config);
void migrate_iwads(nlohmann::json &config);
void migrate_pwads(nlohmann::json &config);
void migrate_doom_executables(nlohmann::json &config);

TEST_CASE("migrate_iwads initializes iwads and selected_iwad if missing")
{
    nlohmann::json config = {};

    migrate_iwads(config);

    CHECK(config.contains("iwads"));
    CHECK(config["iwads"].is_array());
    CHECK(config["iwads"].empty());
    CHECK(config.contains("selected_iwad"));
    CHECK(config["selected_iwad"] == "");
}

TEST_CASE("migrate_iwads migrates iwad_filepath to iwads array")
{
    nlohmann::json config = {
        {"iwad_filepath", "/path/to/doom.wad"}};

    migrate_iwads(config);

    CHECK(config["iwads"].size() == 1);
    CHECK(config["iwads"][0] == "/path/to/doom.wad");
    CHECK(config["selected_iwad"] == "/path/to/doom.wad");
    CHECK_FALSE(config.contains("iwad_filepath"));
}

TEST_CASE("migrate_iwads avoids duplicate IWAD entries")
{
    nlohmann::json config = {
        {"iwads", {"/path/to/doom.wad"}},
        {"iwad_filepath", "/path/to/doom.wad"}};

    migrate_iwads(config);

    CHECK(config["iwads"].size() == 1);
    CHECK(config["iwads"][0] == "/path/to/doom.wad");
    CHECK(config["selected_iwad"] == "/path/to/doom.wad");
    CHECK_FALSE(config.contains("iwad_filepath"));
}

TEST_CASE("migrate_iwads handles empty iwad_filepath")
{
    nlohmann::json config = {
        {"iwad_filepath", ""}};

    migrate_iwads(config);

    CHECK(config["iwads"].empty());
    CHECK(config["selected_iwad"] == "");
    CHECK_FALSE(config.contains("iwad_filepath"));
}

TEST_CASE("migrate_iwads removes iwad_path after migration")
{
    nlohmann::json config = {
        {"iwad_path", "/path/to/doom.wad"}
    };

    migrate_iwads(config);

    // Check that iwad_path is removed
    CHECK_FALSE(config.contains("iwad_path"));

    // Check that the IWAD was not added to the iwads array (since iwad_path is not migrated)
    CHECK(config["iwads"].empty());

    // Check that selected_iwad remains empty
    CHECK(config["selected_iwad"] == "");
}

TEST_CASE("migrate_config removes pwad_path after migration")
{
    nlohmann::json config = {
        {"pwad_path", "/path/to/some_pwad.wad"}
    };

    migrate_pwads(config);

    // Check that pwad_path is removed
    CHECK_FALSE(config.contains("pwad_path"));

    // Check that no other changes were made to the config
    CHECK(config.empty());
}

TEST_CASE("migrate_config handles doom_executables migration")
{
    nlohmann::json config = {
        {"gzdoom_filepath", "/path/to/gzdoom"}
    };

    migrate_doom_executables(config);

    // Check that gzdoom_filepath is migrated
    CHECK(config["doom_executables"].size() == 1);
    CHECK(config["doom_executables"][0] == "/path/to/gzdoom");
    CHECK_FALSE(config.contains("gzdoom_filepath"));
}

TEST_CASE("migrate_pwads removes pwad_path after migration")
{
    nlohmann::json config = {
        {"pwad_path", "/path/to/some_pwad.wad"}
    };

    migrate_pwads(config);

    // Check that pwad_path is removed
    CHECK_FALSE(config.contains("pwad_path"));

    // Ensure no other changes were made to the config
    CHECK(config.empty());
}

TEST_CASE("migrate_doom_executables initializes doom_executables if missing")
{
    nlohmann::json config = {};

    migrate_doom_executables(config);

    CHECK(config.contains("doom_executables"));
    CHECK(config["doom_executables"].is_array());
    CHECK(config["doom_executables"].empty());
}

TEST_CASE("migrate_doom_executables migrates gzdoom_filepath to doom_executables")
{
    nlohmann::json config = {
        {"gzdoom_filepath", "/path/to/gzdoom"}
    };

    migrate_doom_executables(config);

    CHECK(config["doom_executables"].size() == 1);
    CHECK(config["doom_executables"][0] == "/path/to/gzdoom");
    CHECK_FALSE(config.contains("gzdoom_filepath"));
}

TEST_CASE("migrate_doom_executables avoids duplicate entries")
{
    nlohmann::json config = {
        {"doom_executables", {"/path/to/gzdoom"}},
        {"gzdoom_filepath", "/path/to/gzdoom"}
    };

    migrate_doom_executables(config);

    CHECK(config["doom_executables"].size() == 1);
    CHECK(config["doom_executables"][0] == "/path/to/gzdoom");
    CHECK_FALSE(config.contains("gzdoom_filepath"));
}

// Window size persistence tests
TEST_CASE("Window size validation works correctly")
{
    // Test valid window sizes
    CHECK(validate_window_size(800, 600) == true);
    CHECK(validate_window_size(1920, 1080) == true);
    CHECK(validate_window_size(400, 300) == true);  // Minimum valid
    CHECK(validate_window_size(4096, 4096) == true); // Maximum valid
    
    // Test invalid window sizes
    CHECK(validate_window_size(399, 300) == false);  // Too narrow
    CHECK(validate_window_size(400, 299) == false);  // Too short
    CHECK(validate_window_size(4097, 4096) == false); // Too wide
    CHECK(validate_window_size(4096, 4097) == false); // Too tall
    CHECK(validate_window_size(0, 0) == false);      // Zero size
    CHECK(validate_window_size(-100, -100) == false); // Negative size
}

TEST_CASE("Window size persistence in config")
{
    // Create a test config with window size
    nlohmann::json test_config = {
        {"resolution", {1024, 768}},
        {"theme", "fire"},
        {"custom_params", ""}
    };
    
    // Verify the resolution is stored correctly
    CHECK(test_config["resolution"][0] == 1024);
    CHECK(test_config["resolution"][1] == 768);
    
    // Simulate window resize
    test_config["resolution"] = {1280, 720};
    
    // Verify the new resolution is stored correctly
    CHECK(test_config["resolution"][0] == 1280);
    CHECK(test_config["resolution"][1] == 720);
}

TEST_CASE("SDL renderer setting handles missing field correctly") 
{
    // Test config missing sdl_renderer field (simulates old config)
    nlohmann::json old_config = {
        {"resolution", {800, 600}},
        {"theme", "fire"},
        {"custom_params", ""}
    };
    
    // Verify field doesn't exist initially
    CHECK_FALSE(old_config.contains("sdl_renderer"));
    
    // Simulate setup_config_file() behavior for missing sdl_renderer
    if (!old_config.contains("sdl_renderer") || old_config["sdl_renderer"].is_null())
    {
        old_config["sdl_renderer"] = "auto";
    }
    
    // Verify default value is set
    CHECK(old_config.contains("sdl_renderer"));
    CHECK(old_config["sdl_renderer"] == "auto");
}

TEST_CASE("SDL renderer setting stores any valid string")
{
    nlohmann::json config = {{"sdl_renderer", "auto"}};
    
    // Test that config can store common SDL renderer values
    std::vector<std::string> common_renderers = {"auto", "opengl", "metal", "direct3d", "direct3d11", "software"};
    
    for (const auto& renderer : common_renderers) {
        config["sdl_renderer"] = renderer;
        CHECK(config["sdl_renderer"] == renderer);
    }
    
    // Test that config can store any string (actual validation happens via SDL at runtime)
    config["sdl_renderer"] = "custom_renderer";
    CHECK(config["sdl_renderer"] == "custom_renderer");
    
    // Test empty string
    config["sdl_renderer"] = "";
    CHECK(config["sdl_renderer"] == "");
}

TEST_CASE("Config file save and load preserves window size")
{
    // Create a temporary config file for testing
    std::string temp_config_path = "/tmp/test_just_launch_doom_config.json";
    
    // Create test config with specific window size
    nlohmann::json test_config = {
        {"resolution", {1366, 768}},
        {"theme", "dark"},
        {"custom_params", ""},
        {"pwad_directories", nlohmann::json::array()},
        {"iwads", nlohmann::json::array()},
        {"selected_iwad", ""},
        {"selected_pwads", nlohmann::json::array()},
        {"config_files", nlohmann::json::array()},
        {"selected_config", ""},
        {"doom_executables", nlohmann::json::array()},
        {"selected_executable", ""}
    };
    
    // Write config to file
    bool write_success = write_config_file(temp_config_path, test_config);
    CHECK(write_success);
    
    // Read config back from file
    nlohmann::json loaded_config;
    bool read_success = read_config_file(temp_config_path, loaded_config);
    CHECK(read_success);
    
    // Verify window size was preserved
    CHECK(loaded_config["resolution"][0] == 1366);
    CHECK(loaded_config["resolution"][1] == 768);
    
    // Clean up test file
    std::remove(temp_config_path.c_str());
}

TEST_CASE("group_pwads_by_directory save/load round-trip preserves value")
{
    std::string temp_config_path = "/tmp/test_group_pwads_config.json";

    // Test with false
    nlohmann::json test_config = {
        {"resolution", {800, 600}},
        {"theme", "fire"},
        {"custom_params", ""},
        {"pwad_directories", nlohmann::json::array()},
        {"iwads", nlohmann::json::array()},
        {"selected_iwad", ""},
        {"selected_pwads", nlohmann::json::array()},
        {"config_files", nlohmann::json::array()},
        {"selected_config", ""},
        {"doom_executables", nlohmann::json::array()},
        {"selected_executable", ""},
        {"group_pwads_by_directory", false}
    };

    bool write_success = write_config_file(temp_config_path, test_config);
    CHECK(write_success);

    nlohmann::json loaded_config;
    bool read_success = read_config_file(temp_config_path, loaded_config);
    CHECK(read_success);
    CHECK(loaded_config["group_pwads_by_directory"] == false);

    // Test with true
    test_config["group_pwads_by_directory"] = true;
    write_config_file(temp_config_path, test_config);
    read_config_file(temp_config_path, loaded_config);
    CHECK(loaded_config["group_pwads_by_directory"] == true);

    std::remove(temp_config_path.c_str());
}

TEST_CASE("Config without group_pwads_by_directory gains default on save")
{
    std::string temp_config_path = "/tmp/test_group_pwads_missing.json";

    // Write a config that doesn't have the field (simulates old config)
    nlohmann::json old_config = {
        {"resolution", {800, 600}},
        {"theme", "fire"},
        {"custom_params", ""}
    };

    write_config_file(temp_config_path, old_config);

    // Load it back — field should be missing
    nlohmann::json loaded_config;
    read_config_file(temp_config_path, loaded_config);
    CHECK_FALSE(loaded_config.contains("group_pwads_by_directory"));

    // Add the default (what setup_config_file does on startup)
    loaded_config["group_pwads_by_directory"] = true;

    // Save and reload — field should now persist
    write_config_file(temp_config_path, loaded_config);
    nlohmann::json reloaded_config;
    read_config_file(temp_config_path, reloaded_config);
    CHECK(reloaded_config.contains("group_pwads_by_directory"));
    CHECK(reloaded_config["group_pwads_by_directory"] == true);

    std::remove(temp_config_path.c_str());
}

TEST_CASE("Default window size fallback")
{
    // Test that invalid config values fall back to defaults
    int width = 100, height = 50; // Invalid small size
    apply_window_size_defaults(width, height);
    
    CHECK(width == 800);
    CHECK(height == 600);
    
    // Test that valid sizes remain unchanged
    width = 1024; height = 768;
    apply_window_size_defaults(width, height);
    
    CHECK(width == 1024);
    CHECK(height == 768);
}