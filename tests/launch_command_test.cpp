#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <string>
#include <vector>
#include <algorithm>

// Replicate the extension constants from main.cpp
const std::vector<std::string> WAD_EXTENSIONS = {
    ".wad", ".iwad", ".pwad", ".kpf", ".pk3", ".pk4", ".pk7",
    ".pke", ".lmp", ".mus", ".doom"};
const std::vector<std::string> DEH_EXTENSIONS = {".deh", ".bex", ".hhe"};
const std::vector<std::string> EDF_EXTENSIONS = {".edf"};

// Replicate has_extension from main.cpp
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

// Simplified version of get_launch_command for testing argument grouping
struct TestPwadFile
{
    std::string filepath;
    bool selected;
};

std::string build_launch_args(const std::vector<TestPwadFile> &pwads)
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

TEST_CASE("has_extension matches WAD extensions")
{
    CHECK(has_extension("test.wad", WAD_EXTENSIONS));
    CHECK(has_extension("test.pk3", WAD_EXTENSIONS));
    CHECK(has_extension("test.lmp", WAD_EXTENSIONS));
    CHECK_FALSE(has_extension("test.deh", WAD_EXTENSIONS));
    CHECK_FALSE(has_extension("test.bex", WAD_EXTENSIONS));
    CHECK_FALSE(has_extension("test.edf", WAD_EXTENSIONS));
}

TEST_CASE("has_extension matches DEH extensions")
{
    CHECK(has_extension("patch.deh", DEH_EXTENSIONS));
    CHECK(has_extension("patch.bex", DEH_EXTENSIONS));
    CHECK(has_extension("patch.hhe", DEH_EXTENSIONS));
    CHECK_FALSE(has_extension("test.wad", DEH_EXTENSIONS));
    CHECK_FALSE(has_extension("test.edf", DEH_EXTENSIONS));
}

TEST_CASE("has_extension matches EDF extensions")
{
    CHECK(has_extension("root.edf", EDF_EXTENSIONS));
    CHECK_FALSE(has_extension("test.wad", EDF_EXTENSIONS));
    CHECK_FALSE(has_extension("test.deh", EDF_EXTENSIONS));
}

TEST_CASE("has_extension is case-insensitive")
{
    CHECK(has_extension("TEST.WAD", WAD_EXTENSIONS));
    CHECK(has_extension("PATCH.DEH", DEH_EXTENSIONS));
    CHECK(has_extension("Patch.BEX", DEH_EXTENSIONS));
    CHECK(has_extension("ROOT.EDF", EDF_EXTENSIONS));
}

TEST_CASE("Regular WAD files use -file flag")
{
    std::vector<TestPwadFile> files = {
        {"maps.wad", true},
        {"textures.pk3", true},
    };

    std::string result = build_launch_args(files);
    CHECK(result.find("-file") != std::string::npos);
    CHECK(result.find("-deh") == std::string::npos);
    CHECK(result.find("-edf") == std::string::npos);
}

TEST_CASE("DEH/BEX/HHE files use -deh flag")
{
    std::vector<TestPwadFile> files = {
        {"patch.deh", true},
        {"another.bex", true},
        {"heretic.hhe", true},
    };

    std::string result = build_launch_args(files);
    CHECK(result.find("-deh") != std::string::npos);
    CHECK(result.find("-file") == std::string::npos);
    CHECK(result.find("-edf") == std::string::npos);
    CHECK(result.find("patch.deh") != std::string::npos);
    CHECK(result.find("another.bex") != std::string::npos);
    CHECK(result.find("heretic.hhe") != std::string::npos);
}

TEST_CASE("EDF files use -edf flag")
{
    std::vector<TestPwadFile> files = {
        {"root.edf", true},
    };

    std::string result = build_launch_args(files);
    CHECK(result.find("-edf") != std::string::npos);
    CHECK(result.find("-file") == std::string::npos);
    CHECK(result.find("-deh") == std::string::npos);
}

TEST_CASE("Mixed file types produce correct grouped arguments")
{
    std::vector<TestPwadFile> files = {
        {"maps.wad", true},
        {"patch.deh", true},
        {"textures.pk3", true},
        {"root.edf", true},
        {"another.bex", true},
    };

    std::string result = build_launch_args(files);

    // All three flags should be present
    CHECK(result.find("-file") != std::string::npos);
    CHECK(result.find("-deh") != std::string::npos);
    CHECK(result.find("-edf") != std::string::npos);

    // -file should come before -deh, which should come before -edf
    size_t file_pos = result.find("-file");
    size_t deh_pos = result.find("-deh");
    size_t edf_pos = result.find("-edf");
    CHECK(file_pos < deh_pos);
    CHECK(deh_pos < edf_pos);

    // WAD files should be grouped under -file
    CHECK(result.find("maps.wad") != std::string::npos);
    CHECK(result.find("textures.pk3") != std::string::npos);

    // DEH files should be grouped under -deh
    CHECK(result.find("patch.deh") != std::string::npos);
    CHECK(result.find("another.bex") != std::string::npos);

    // EDF files should be grouped under -edf
    CHECK(result.find("root.edf") != std::string::npos);
}

TEST_CASE("Unselected files are not included")
{
    std::vector<TestPwadFile> files = {
        {"maps.wad", false},
        {"patch.deh", false},
        {"root.edf", false},
    };

    std::string result = build_launch_args(files);
    CHECK(result.empty());
}
