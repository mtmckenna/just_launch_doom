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
    std::vector<std::string> files = {"maps.wad", "textures.pk3"};

    std::string result = build_launch_file_args(files);
    CHECK(result.find("-file") != std::string::npos);
    CHECK(result.find("-deh") == std::string::npos);
    CHECK(result.find("-edf") == std::string::npos);
}

TEST_CASE("DEH/BEX/HHE files use -deh flag")
{
    std::vector<std::string> files = {"patch.deh", "another.bex", "heretic.hhe"};

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
    std::vector<std::string> files = {"root.edf"};

    std::string result = build_launch_file_args(files);
    CHECK(result.find("-edf") != std::string::npos);
    CHECK(result.find("-file") == std::string::npos);
    CHECK(result.find("-deh") == std::string::npos);
}

TEST_CASE("Mixed file types produce correct grouped arguments")
{
    std::vector<std::string> files = {
        "maps.wad", "patch.deh", "textures.pk3", "root.edf", "another.bex"};

    std::string result = build_launch_file_args(files);

    // All three flags should be present
    CHECK(result.find("-file") != std::string::npos);
    CHECK(result.find("-deh") != std::string::npos);
    CHECK(result.find("-edf") != std::string::npos);

    // Flags appear in order of first selection: maps.wad (-file) first, patch.deh (-deh) second, root.edf (-edf) third
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

TEST_CASE("Flag order follows selection order across different flag types")
{
    // DEH selected first, then WAD — should produce -deh before -file
    std::vector<std::string> deh_first = {"bloodcolor.deh", "cblood.pk3"};
    std::string result = build_launch_file_args(deh_first);
    CHECK(result.find("-deh") < result.find("-file"));

    // WAD selected first, then DEH — should produce -file before -deh
    std::vector<std::string> wad_first = {"cblood.pk3", "bloodcolor.deh"};
    result = build_launch_file_args(wad_first);
    CHECK(result.find("-file") < result.find("-deh"));

    // EDF selected first, then WAD — should produce -edf before -file
    std::vector<std::string> edf_first = {"root.edf", "maps.wad"};
    result = build_launch_file_args(edf_first);
    CHECK(result.find("-edf") < result.find("-file"));
}

TEST_CASE("Empty file list produces empty result")
{
    std::vector<std::string> files = {};

    std::string result = build_launch_file_args(files);
    CHECK(result.empty());
}

TEST_CASE("File order is preserved within each flag group")
{
    std::vector<std::string> files = {"z_last.wad", "a_first.wad", "m_middle.wad"};

    std::string result = build_launch_file_args(files);

    // Files should appear in the order they were passed, not alphabetically
    size_t z_pos = result.find("z_last.wad");
    size_t a_pos = result.find("a_first.wad");
    size_t m_pos = result.find("m_middle.wad");
    CHECK(z_pos < a_pos);
    CHECK(a_pos < m_pos);
}

TEST_CASE("build_display_names returns filename for unique paths")
{
    std::vector<std::string> paths = {"/dir1/doom.wad", "/dir2/heretic.wad"};
    auto names = build_display_names(paths);
    CHECK(names["/dir1/doom.wad"] == "doom.wad");
    CHECK(names["/dir2/heretic.wad"] == "heretic.wad");
}

TEST_CASE("build_display_names disambiguates duplicate filenames with numbers")
{
    std::vector<std::string> paths = {"/beta/heretic.wad", "/alpha/heretic.wad"};
    auto names = build_display_names(paths);
    // Alphabetical by full path: /alpha/... gets (1), /beta/... gets (2)
    CHECK(names["/alpha/heretic.wad"] == "heretic.wad (1)");
    CHECK(names["/beta/heretic.wad"] == "heretic.wad (2)");
}

TEST_CASE("build_display_names handles mix of unique and duplicate filenames")
{
    std::vector<std::string> paths = {"/dir1/doom.wad", "/dir2/doom.wad", "/dir3/heretic.wad"};
    auto names = build_display_names(paths);
    CHECK(names["/dir1/doom.wad"] == "doom.wad (1)");
    CHECK(names["/dir2/doom.wad"] == "doom.wad (2)");
    CHECK(names["/dir3/heretic.wad"] == "heretic.wad");
}

TEST_CASE("build_display_names handles empty input")
{
    std::vector<std::string> paths = {};
    auto names = build_display_names(paths);
    CHECK(names.empty());
}
