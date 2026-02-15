#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/launch_utils.h"

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
    std::vector<PwadFileInfo> files = {
        {"maps.wad", true, ""},
        {"textures.pk3", true, ""},
    };

    std::string result = build_launch_file_args(files);
    CHECK(result.find("-file") != std::string::npos);
    CHECK(result.find("-deh") == std::string::npos);
    CHECK(result.find("-edf") == std::string::npos);
}

TEST_CASE("DEH/BEX/HHE files use -deh flag")
{
    std::vector<PwadFileInfo> files = {
        {"patch.deh", true, ""},
        {"another.bex", true, ""},
        {"heretic.hhe", true, ""},
    };

    std::string result = build_launch_file_args(files);
    CHECK(result.find("-deh") != std::string::npos);
    CHECK(result.find("-file") == std::string::npos);
    CHECK(result.find("-edf") == std::string::npos);
    CHECK(result.find("patch.deh") != std::string::npos);
    CHECK(result.find("another.bex") != std::string::npos);
    CHECK(result.find("heretic.hhe") != std::string::npos);
}

TEST_CASE("EDF files use -edf flag")
{
    std::vector<PwadFileInfo> files = {
        {"root.edf", true, ""},
    };

    std::string result = build_launch_file_args(files);
    CHECK(result.find("-edf") != std::string::npos);
    CHECK(result.find("-file") == std::string::npos);
    CHECK(result.find("-deh") == std::string::npos);
}

TEST_CASE("Mixed file types produce correct grouped arguments")
{
    std::vector<PwadFileInfo> files = {
        {"maps.wad", true, ""},
        {"patch.deh", true, ""},
        {"textures.pk3", true, ""},
        {"root.edf", true, ""},
        {"another.bex", true, ""},
    };

    std::string result = build_launch_file_args(files);

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
    std::vector<PwadFileInfo> files = {
        {"maps.wad", false, ""},
        {"patch.deh", false, ""},
        {"root.edf", false, ""},
    };

    std::string result = build_launch_file_args(files);
    CHECK(result.empty());
}
