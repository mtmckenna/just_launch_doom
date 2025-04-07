#include "nlohmann/json.hpp"

nlohmann::json migrate_config(nlohmann::json config)
{
    // Initialize doom_executables array if it doesn't exist
    if (!config.contains("doom_executables"))
    {
        config["doom_executables"] = nlohmann::json::array();
    }

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
                config["selected_iwad"] = filepath;
            }
        }

        // Only erase if it exists
        config.erase("iwad_filepath");
    }

    return config;
}