#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// Mock the PwadFileInfo structure and functions we need to test
struct PwadFileInfo {
    std::string filepath;
    bool selected;
    std::string txt_filepath;
};

// Function to test text file detection logic (extracted from populate_pwad_list)
std::string detect_companion_txt_file(const std::string& pwad_filepath) {
    fs::path pwad_path(pwad_filepath);
    std::string base_name = pwad_path.stem().string(); // Get filename without extension
    std::string txt_path = (pwad_path.parent_path() / (base_name + ".txt")).string();
    
    if (fs::exists(txt_path)) {
        return txt_path;
    }
    return "";
}

TEST_CASE("Text file detection works correctly") {
    // Create temporary test directory
    std::string test_dir = "/tmp/just_launch_doom_test";
    fs::create_directories(test_dir);
    
    SUBCASE("Detects existing companion text file") {
        // Create test PWAD and text files
        std::string pwad_path = test_dir + "/test.wad";
        std::string txt_path = test_dir + "/test.txt";
        
        std::ofstream pwad_file(pwad_path);
        pwad_file << "fake pwad content";
        pwad_file.close();
        
        std::ofstream txt_file(txt_path);
        txt_file << "This is a test text file for the PWAD.";
        txt_file.close();
        
        std::string detected_txt = detect_companion_txt_file(pwad_path);
        CHECK(detected_txt == txt_path);
        CHECK(fs::exists(detected_txt));
    }
    
    SUBCASE("Returns empty string when no companion text file exists") {
        std::string pwad_path = test_dir + "/no_txt.wad";
        
        std::ofstream pwad_file(pwad_path);
        pwad_file << "fake pwad content";
        pwad_file.close();
        
        std::string detected_txt = detect_companion_txt_file(pwad_path);
        CHECK(detected_txt.empty());
    }
    
    SUBCASE("Works with different PWAD extensions") {
        std::vector<std::string> extensions = {".pk3", ".pk4", ".pk7", ".iwad"};
        
        for (const auto& ext : extensions) {
            std::string pwad_path = test_dir + "/test" + ext;
            std::string txt_path = test_dir + "/test.txt";
            
            std::ofstream pwad_file(pwad_path);
            pwad_file << "fake content";
            pwad_file.close();
            
            std::ofstream txt_file(txt_path);
            txt_file << "companion text";
            txt_file.close();
            
            std::string detected_txt = detect_companion_txt_file(pwad_path);
            CHECK(detected_txt == txt_path);
            
            // Clean up for next iteration
            fs::remove(pwad_path);
            fs::remove(txt_path);
        }
    }
    
    SUBCASE("Handles files with complex names") {
        std::string pwad_path = test_dir + "/doom2-map01-v2.1.wad";
        std::string txt_path = test_dir + "/doom2-map01-v2.1.txt";
        
        std::ofstream pwad_file(pwad_path);
        pwad_file << "fake pwad content";
        pwad_file.close();
        
        std::ofstream txt_file(txt_path);
        txt_file << "readme for doom2-map01-v2.1";
        txt_file.close();
        
        std::string detected_txt = detect_companion_txt_file(pwad_path);
        CHECK(detected_txt == txt_path);
    }
    
    SUBCASE("Case sensitivity test") {
        std::string pwad_path = test_dir + "/Test.WAD";
        std::string txt_path = test_dir + "/Test.txt";
        
        std::ofstream pwad_file(pwad_path);
        pwad_file << "fake pwad content";
        pwad_file.close();
        
        std::ofstream txt_file(txt_path);
        txt_file << "test text content";
        txt_file.close();
        
        std::string detected_txt = detect_companion_txt_file(pwad_path);
        CHECK(detected_txt == txt_path);
    }
    
    // Clean up test directory
    fs::remove_all(test_dir);
}

TEST_CASE("PwadFileInfo structure works correctly") {
    SUBCASE("Structure can be initialized properly") {
        PwadFileInfo info = {"/path/to/doom.wad", true, "/path/to/doom.txt"};
        
        CHECK(info.filepath == "/path/to/doom.wad");
        CHECK(info.selected == true);
        CHECK(info.txt_filepath == "/path/to/doom.txt");
    }
    
    SUBCASE("Structure works with empty text file path") {
        PwadFileInfo info = {"/path/to/doom.wad", false, ""};
        
        CHECK(info.filepath == "/path/to/doom.wad");
        CHECK(info.selected == false);
        CHECK(info.txt_filepath.empty());
    }
    
    SUBCASE("Vector of PwadFileInfo works correctly") {
        std::vector<PwadFileInfo> pwads;
        pwads.push_back({"/path/doom.wad", true, "/path/doom.txt"});
        pwads.push_back({"/path/heretic.wad", false, ""});
        
        CHECK(pwads.size() == 2);
        CHECK(pwads[0].filepath == "/path/doom.wad");
        CHECK(pwads[0].selected == true);
        CHECK(!pwads[0].txt_filepath.empty());
        CHECK(pwads[1].txt_filepath.empty());
    }
}

TEST_CASE("File path handling edge cases") {
    SUBCASE("Handles paths with spaces") {
        std::string test_dir = "/tmp/test with spaces";
        fs::create_directories(test_dir);
        
        std::string pwad_path = test_dir + "/file with spaces.wad";
        std::string txt_path = test_dir + "/file with spaces.txt";
        
        std::ofstream pwad_file(pwad_path);
        pwad_file << "content";
        pwad_file.close();
        
        std::ofstream txt_file(txt_path);
        txt_file << "text content";
        txt_file.close();
        
        std::string detected_txt = detect_companion_txt_file(pwad_path);
        CHECK(detected_txt == txt_path);
        
        fs::remove_all(test_dir);
    }
    
    SUBCASE("Handles paths with special characters") {
        std::string test_dir = "/tmp/test-dir_v2.1";
        fs::create_directories(test_dir);
        
        std::string pwad_path = test_dir + "/mod-v1.2_final.pk3";
        std::string txt_path = test_dir + "/mod-v1.2_final.txt";
        
        std::ofstream pwad_file(pwad_path);
        pwad_file << "content";
        pwad_file.close();
        
        std::ofstream txt_file(txt_path);
        txt_file << "text content";
        txt_file.close();
        
        std::string detected_txt = detect_companion_txt_file(pwad_path);
        CHECK(detected_txt == txt_path);
        
        fs::remove_all(test_dir);
    }
}