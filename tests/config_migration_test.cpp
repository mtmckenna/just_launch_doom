#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/nlohmann/json.hpp"
#include <filesystem>

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