#include "nlohmann/json.hpp"

nlohmann::json migrate_config(nlohmann::json config)
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

    return config;
}