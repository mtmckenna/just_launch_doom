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

std::string build_launch_file_args(const std::vector<std::string> &selected_paths)
{
    auto get_flag = [](const std::string &filepath) -> std::string
    {
        if (has_extension(filepath, DEH_EXTENSIONS)) return "-deh";
        if (has_extension(filepath, EDF_EXTENSIONS)) return "-edf";
        return "-file";
    };

    std::vector<std::string> flag_order;
    std::map<std::string, std::string> flag_files;

    for (const auto &filepath : selected_paths)
    {
        std::string flag = get_flag(filepath);
        std::string quoted = "\"" + filepath + "\" ";

        if (flag_files.find(flag) == flag_files.end())
        {
            flag_order.push_back(flag);
            flag_files[flag] = "";
        }
        flag_files[flag] += quoted;
    }

    std::string result = "";
    for (const auto &flag : flag_order)
    {
        result += " " + flag + " " + flag_files[flag];
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

// Normalize a path for comparison: collapse "." / ".." segments, use forward
// slashes, drop any trailing separator, and lowercase on Windows (where paths
// are case-insensitive).
static std::string normalize_for_comparison(const std::string &path)
{
    std::string normalized = std::filesystem::path(path).lexically_normal().generic_string();

    while (normalized.length() > 1 && normalized.back() == '/')
    {
        normalized.pop_back();
    }

#ifdef _WIN32
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
#endif

    return normalized;
}

bool is_path_in_directory(const std::string &filepath, const std::string &directory)
{
    if (filepath.empty() || directory.empty())
    {
        return false;
    }

    // PWAD directories are scanned non-recursively, so only direct children of
    // the directory belong to it. A nested directory that was added separately
    // keeps its own files.
    std::string parent = std::filesystem::path(filepath).parent_path().string();
    return normalize_for_comparison(parent) == normalize_for_comparison(directory);
}

std::vector<std::string> remove_pwads_in_directory(const std::vector<std::string> &selected_paths, const std::string &directory)
{
    std::vector<std::string> remaining;

    for (const auto &filepath : selected_paths)
    {
        if (!is_path_in_directory(filepath, directory))
        {
            remaining.push_back(filepath);
        }
    }

    return remaining;
}
