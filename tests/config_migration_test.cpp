#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/nlohmann/json.hpp"
#include <filesystem>

// Include the config migration function from config_migration.cpp
nlohmann::json migrate_config(nlohmann::json config);
void migrate_iwads(nlohmann::json &config);
void migrate_pwads(nlohmann::json &config);

TEST_CASE("Config Migration")
{
    // Test Case 1: Empty Config
    // Tests that when given an empty config object:
    // - The doom_executables array is initialized
    // - No gzdoom_filepath field exists
    // - The config is otherwise unchanged
    SUBCASE("Empty config gets all required fields")
    {
        nlohmann::json config;
        nlohmann::json migrated = migrate_config(config);

        CHECK(migrated.contains("doom_executables"));
        CHECK(migrated["doom_executables"].is_array());
        CHECK(!migrated.contains("gzdoom_filepath"));
    }

    // Test Case 2: Simple Migration
    // Tests that when given a config with only gzdoom_filepath:
    // - The path is moved to doom_executables array
    // - The gzdoom_filepath field is removed
    // - The array contains exactly one entry
    SUBCASE("Config with gzdoom_filepath migrates to doom_executables")
    {
        nlohmann::json config = {
            {"gzdoom_filepath", "/path/to/gzdoom"}};

        nlohmann::json migrated = migrate_config(config);

        CHECK(migrated.contains("doom_executables"));
        CHECK(migrated["doom_executables"].is_array());
        CHECK(migrated["doom_executables"].size() == 1);
        CHECK(migrated["doom_executables"][0] == "/path/to/gzdoom");
        CHECK(!migrated.contains("gzdoom_filepath"));
    }

    // Test Case 3: Merge with Existing
    // Tests that when given a config with both gzdoom_filepath and existing doom_executables:
    // - The new path is added to the existing array
    // - The original entries are preserved
    // - The gzdoom_filepath field is removed
    SUBCASE("Config with existing doom_executables adds gzdoom_filepath if not present")
    {
        nlohmann::json config = {
            {"gzdoom_filepath", "/path/to/gzdoom"},
            {"doom_executables", {"/path/to/other"}}};

        nlohmann::json migrated = migrate_config(config);

        CHECK(migrated["doom_executables"].size() == 2);
        CHECK(migrated["doom_executables"][0] == "/path/to/other");
        CHECK(migrated["doom_executables"][1] == "/path/to/gzdoom");
        CHECK(!migrated.contains("gzdoom_filepath"));
    }

    // Test Case 4: Duplicate Prevention
    // Tests that when given a config where gzdoom_filepath already exists in doom_executables:
    // - The path is not added again
    // - The array size remains unchanged
    // - The gzdoom_filepath field is removed
    SUBCASE("Config with existing doom_executables doesn't add duplicate gzdoom_filepath")
    {
        nlohmann::json config = {
            {"gzdoom_filepath", "/path/to/gzdoom"},
            {"doom_executables", {"/path/to/gzdoom"}}};

        nlohmann::json migrated = migrate_config(config);

        CHECK(migrated["doom_executables"].size() == 1);
        CHECK(migrated["doom_executables"][0] == "/path/to/gzdoom");
        CHECK(!migrated.contains("gzdoom_filepath"));
    }
}

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

    nlohmann::json migrated = migrate_config(config);

    // Check that pwad_path is removed
    CHECK_FALSE(migrated.contains("pwad_path"));

    // Check that no other changes were made to the config
    CHECK(migrated["doom_executables"].is_array());
    CHECK(migrated["iwads"].is_array());
    CHECK(migrated["selected_iwad"] == "");
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