#include "launch_utils.h"
#include <algorithm>
#include <filesystem>

const std::vector<std::string> WAD_EXTENSIONS = {
    ".wad", ".iwad", ".pwad", ".kpf", ".pk3", ".pk4", ".pk7",
    ".pke", ".lmp", ".mus", ".doom"};
const std::vector<std::string> DEH_EXTENSIONS = {".deh", ".bex", ".hhe"};
const std::vector<std::string> EDF_EXTENSIONS = {".edf"};

bool has_extension(const std::string &filepath, const std::vector<std::string> &extensions)
{
    std::string lower_filepath = filepath;
    std::transform(lower_filepath.begin(), lower_filepath.end(), lower_filepath.begin(), ::tolower);
    return std::any_of(extensions.begin(), extensions.end(),
                       [&lower_filepath](const std::string &ext)
                       {
                           return lower_filepath.length() >= ext.length() &&
                                  lower_filepath.substr(lower_filepath.length() - ext.length()) == ext;
                       });
}

std::string build_launch_file_args(const std::vector<PwadFileInfo> &pwads)
{
    std::string wad_files = "";
    std::string deh_files = "";
    std::string edf_files = "";

    for (size_t i = 0; i < pwads.size(); i++)
    {
        if (pwads[i].selected)
        {
            std::string quoted = "\"" + pwads[i].filepath + "\" ";
            if (has_extension(pwads[i].filepath, DEH_EXTENSIONS))
            {
                deh_files += quoted;
            }
            else if (has_extension(pwads[i].filepath, EDF_EXTENSIONS))
            {
                edf_files += quoted;
            }
            else
            {
                wad_files += quoted;
            }
        }
    }

    std::string result = "";
    if (!wad_files.empty())
    {
        result += " -file " + wad_files;
    }
    if (!deh_files.empty())
    {
        result += " -deh " + deh_files;
    }
    if (!edf_files.empty())
    {
        result += " -edf " + edf_files;
    }

    return result;
}

std::map<std::string, std::string> build_display_names(const std::vector<std::string> &paths)
{
    std::map<std::string, std::string> display_names;
    std::map<std::string, int> filename_counts;

    for (const auto &path : paths)
    {
        std::string filename = std::filesystem::path(path).filename().string();
        filename_counts[filename]++;
    }

    std::map<std::string, std::vector<std::string>> filename_paths;
    for (const auto &path : paths)
    {
        std::string filename = std::filesystem::path(path).filename().string();
        filename_paths[filename].push_back(path);
    }

    for (auto &[filename, grouped_paths] : filename_paths)
    {
        std::sort(grouped_paths.begin(), grouped_paths.end());
        for (size_t j = 0; j < grouped_paths.size(); j++)
        {
            if (grouped_paths.size() > 1)
            {
                display_names[grouped_paths[j]] = filename + " (" + std::to_string(j + 1) + ")";
            }
            else
            {
                display_names[grouped_paths[j]] = filename;
            }
        }
    }

    return display_names;
}
