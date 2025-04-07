#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/nlohmann/json.hpp"
#include <filesystem>

// Include the config migration function from config_migration.cpp
extern nlohmann::json migrate_config(nlohmann::json config);

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