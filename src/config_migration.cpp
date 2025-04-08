#include "nlohmann/json.hpp"

// Function to migrate IWAD-related configuration
void migrate_iwads(nlohmann::json &config)
{
    // Initialize iwads array if it doesn't exist
    if (!config.contains("iwads"))
    {
        config["iwads"] = nlohmann::json::array();
    }

    // Initialize selected_iwad if it doesn't exist
    if (!config.contains("selected_iwad"))
    {
        config["selected_iwad"] = "";
    }

    // Migrate iwad_filepath to iwads array if it exists
    if (config.contains("iwad_filepath") && !config["iwad_filepath"].is_null())
    {
        std::string filepath = config["iwad_filepath"].get<std::string>();

        if (!filepath.empty())
        {
            bool found = false;
            for (const auto &iwad : config["iwads"])
            {
                if (iwad.get<std::string>() == filepath)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                config["iwads"].push_back(filepath);
            }

            // Always set selected_iwad to the migrated filepath
            config["selected_iwad"] = filepath;
        }

        // Remove the old iwad_filepath field
        config.erase("iwad_filepath");

        
    }

    // Remove the old iwad_path field if it exists
    if (config.contains("iwad_path"))
    {
        config.erase("iwad_path");
    }
}

void migrate_pwads(nlohmann::json &config)
{
    // Remove the old pwad_path field if it exists
    if (config.contains("pwad_path"))
    {
        config.erase("pwad_path");
    }
}

void migrate_doom_executables(nlohmann::json &config)
{
    // Initialize doom_executables array if it doesn't exist
    if (!config.contains("doom_executables"))
    {
        config["doom_executables"] = nlohmann::json::array();
    }

    // Only try to migrate gzdoom_filepath if it exists
    if (config.contains("gzdoom_filepath") && !config["gzdoom_filepath"].is_null())
    {
        std::string filepath = config["gzdoom_filepath"].get<std::string>();

        if (!filepath.empty())
        {
            bool found = false;
            for (const auto &exe : config["doom_executables"])
            {
                if (exe.get<std::string>() == filepath)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                config["doom_executables"].push_back(filepath);
            }
        }

        // Only erase if it exists
        config.erase("gzdoom_filepath");
    }
}

// Main migration function
nlohmann::json migrate_config(nlohmann::json config)
{
    // Migrate doom_executables-related configuration
    migrate_doom_executables(config);

    // Migrate IWAD-related configuration
    migrate_iwads(config);

    // Migrate PWAD-related configuration
    migrate_pwads(config);

    return config;
}