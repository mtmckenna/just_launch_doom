#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/launch_utils.h"

TEST_CASE("is_path_in_directory matches direct children")
{
    CHECK(is_path_in_directory("/wads/doom2.wad", "/wads"));
    CHECK(is_path_in_directory("/wads/sunlust.wad", "/wads"));
    CHECK_FALSE(is_path_in_directory("/other/doom2.wad", "/wads"));
}

TEST_CASE("is_path_in_directory ignores a trailing separator on the directory")
{
    CHECK(is_path_in_directory("/wads/doom2.wad", "/wads/"));
    CHECK(is_path_in_directory("/wads/doom2.wad", "/wads/."));
}

TEST_CASE("is_path_in_directory does not match nested directories")
{
    // PWAD directories are scanned non-recursively, so a subdirectory that was
    // added separately keeps its own selections.
    CHECK_FALSE(is_path_in_directory("/wads/extra/doom2.wad", "/wads"));
    CHECK(is_path_in_directory("/wads/extra/doom2.wad", "/wads/extra"));
}

TEST_CASE("is_path_in_directory does not match a directory that is a name prefix")
{
    CHECK_FALSE(is_path_in_directory("/wads2/doom2.wad", "/wads"));
    CHECK_FALSE(is_path_in_directory("/wads/doom2.wad", "/wad"));
}

TEST_CASE("is_path_in_directory rejects empty inputs")
{
    CHECK_FALSE(is_path_in_directory("", "/wads"));
    CHECK_FALSE(is_path_in_directory("/wads/doom2.wad", ""));
    CHECK_FALSE(is_path_in_directory("doom2.wad", ""));
}

TEST_CASE("remove_pwads_in_directory drops selections from the removed directory")
{
    std::vector<std::string> selected = {
        "/wads/doom2.wad",
        "/other/sunlust.wad",
        "/wads/valiant.wad"};

    auto remaining = remove_pwads_in_directory(selected, "/wads");

    REQUIRE(remaining.size() == 1);
    CHECK(remaining[0] == "/other/sunlust.wad");
}

TEST_CASE("remove_pwads_in_directory keeps selections from other directories in order")
{
    std::vector<std::string> selected = {
        "/a/first.wad",
        "/wads/doom2.wad",
        "/b/second.wad",
        "/wads/patch.deh",
        "/c/third.wad"};

    auto remaining = remove_pwads_in_directory(selected, "/wads");

    REQUIRE(remaining.size() == 3);
    CHECK(remaining[0] == "/a/first.wad");
    CHECK(remaining[1] == "/b/second.wad");
    CHECK(remaining[2] == "/c/third.wad");
}

TEST_CASE("remove_pwads_in_directory removes every file type in the directory")
{
    std::vector<std::string> selected = {
        "/wads/map.wad",
        "/wads/patch.deh",
        "/wads/root.edf",
        "/wads/mod.pk3"};

    auto remaining = remove_pwads_in_directory(selected, "/wads");

    CHECK(remaining.empty());
}

TEST_CASE("remove_pwads_in_directory leaves the list alone for an unknown directory")
{
    std::vector<std::string> selected = {"/wads/doom2.wad", "/other/sunlust.wad"};

    auto remaining = remove_pwads_in_directory(selected, "/nothing/here");

    REQUIRE(remaining.size() == 2);
    CHECK(remaining[0] == "/wads/doom2.wad");
    CHECK(remaining[1] == "/other/sunlust.wad");
}

TEST_CASE("remove_pwads_in_directory handles an empty selection")
{
    std::vector<std::string> selected;

    CHECK(remove_pwads_in_directory(selected, "/wads").empty());
}

TEST_CASE("The launch command no longer includes PWADs from a removed directory")
{
    std::vector<std::string> selected = {
        "/wads/doom2.wad",
        "/other/sunlust.wad"};

    std::string before = build_launch_file_args(selected);
    CHECK(before.find("doom2.wad") != std::string::npos);

    std::string after = build_launch_file_args(remove_pwads_in_directory(selected, "/wads"));
    CHECK(after.find("doom2.wad") == std::string::npos);
    CHECK(after.find("sunlust.wad") != std::string::npos);
}

#ifdef _WIN32
TEST_CASE("is_path_in_directory compares Windows paths case-insensitively")
{
    CHECK(is_path_in_directory("C:\\Doom\\Wads\\doom2.wad", "c:\\doom\\wads"));
    CHECK(is_path_in_directory("C:\\Doom\\Wads\\doom2.wad", "C:/Doom/Wads"));
    CHECK_FALSE(is_path_in_directory("C:\\Doom\\Other\\doom2.wad", "C:\\Doom\\Wads"));
}

TEST_CASE("remove_pwads_in_directory handles Windows separators")
{
    std::vector<std::string> selected = {
        "C:\\Doom\\Wads\\doom2.wad",
        "D:\\Mods\\sunlust.wad"};

    auto remaining = remove_pwads_in_directory(selected, "C:\\Doom\\Wads\\");

    REQUIRE(remaining.size() == 1);
    CHECK(remaining[0] == "D:\\Mods\\sunlust.wad");
}
#endif
