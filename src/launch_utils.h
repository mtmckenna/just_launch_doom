#pragma once
#include <map>
#include <string>
#include <vector>

struct PwadFileInfo
{
    std::string filepath;
    bool selected;
    std::string txt_filepath; // Empty if no txt file exists
    std::string directory;    // Source directory this file came from
};

extern const std::vector<std::string> WAD_EXTENSIONS;
extern const std::vector<std::string> DEH_EXTENSIONS;
extern const std::vector<std::string> EDF_EXTENSIONS;

bool has_extension(const std::string &filepath, const std::vector<std::string> &extensions);
std::string build_launch_file_args(const std::vector<PwadFileInfo> &pwads);
std::map<std::string, std::string> build_display_names(const std::vector<std::string> &paths);
