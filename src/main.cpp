#include <iostream>
#include <cassert>
#include <fstream>
#include <SDL.h>
#include <filesystem>

#include "nlohmann/json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui-filebrowser/imfilebrowser.h"

#include "fire.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#define VERSION "0.1.4"

int renderer_width, renderer_height;
int color_buffer_width, color_buffer_height;
int launch_button_height = 35;
char command_buf[1024] = "THIS IS THE COMMAND";
char custom_params_buf[1024] = "";
// std::vector<bool> pwads(120, false);
std::vector<std::pair<std::string, bool>> pwads;

uint32_t *color_buffer = nullptr;
nlohmann::json config = {
    {"gzdoom_filepath", ""},
    {"resolution", {800, 600}},
    {"pwad_directories", nlohmann::json::array()},
    {"iwad_filepath", ""},
    {"selected_pwads", nlohmann::json::array()},
    {"custom_params", ""},
    {"theme", "fire"}};

// Theme definitions
struct Theme
{
    ImVec4 button_color;
    ImVec4 button_hover_color;
    ImVec4 button_active_color;
    ImVec4 dialog_title_color;
    ImVec4 clear_color;
    ImVec4 frame_bg_color;
    ImVec4 text_color;       // Add text color
    ImVec4 text_color_green; // For success/selected states
    ImVec4 text_color_red;   // For error/unselected states
};

const std::map<std::string, Theme> themes = {
    {"dark", {
                 ImVec4(0.4f, 0.4f, 0.4f, 1.0f),  // Gray
                 ImVec4(0.5f, 0.5f, 0.5f, 1.0f),  // Lighter gray
                 ImVec4(0.3f, 0.3f, 0.3f, 1.0f),  // Darker gray
                 ImVec4(1.0f, 1.0f, 1.0f, 1.0f),  // White
                 ImVec4(0.1f, 0.1f, 0.1f, 1.00f), // Dark gray
                 ImVec4(0.0f, 0.0f, 0.0f, 0.75f), // Semi-transparent black
                 ImVec4(1.0f, 1.0f, 1.0f, 1.0f),  // White text
                 ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green text
                 ImVec4(1.0f, 0.0f, 0.0f, 1.0f)   // Red text
             }},
    {"fire", {
                 ImVec4(0.85f, 0.46f, 0.14f, 1.0f),  // Orange
                 ImVec4(0.95f, 0.56f, 0.24f, 1.0f),  // Lighter orange
                 ImVec4(0.75f, 0.36f, 0.04f, 1.0f),  // Darker orange
                 ImVec4(0.0f, 0.0f, 0.0f, 1.0f),     // Black
                 ImVec4(0.45f, 0.55f, 0.60f, 1.00f), // Gray
                 ImVec4(0.0f, 0.0f, 0.0f, 0.75f),    // Semi-transparent black
                 ImVec4(1.0f, 1.0f, 1.0f, 1.0f),     // White text
                 ImVec4(0.0f, 1.0f, 0.0f, 1.0f),     // Green text
                 ImVec4(1.0f, 0.0f, 0.0f, 1.0f)      // Red text
             }},
    {"light", {
                  ImVec4(0.53f, 0.71f, 0.94f, 1.0f),  // Light blue for buttons
                  ImVec4(0.63f, 0.81f, 0.94f, 1.0f),  // Lighter blue for hover
                  ImVec4(0.43f, 0.61f, 0.94f, 1.0f),  // Darker blue for active
                  ImVec4(0.0f, 0.0f, 0.0f, 1.0f),     // Black for dialog title
                  ImVec4(0.90f, 0.90f, 0.90f, 1.00f), // Light gray for background (softer)
                  ImVec4(0.94f, 0.94f, 0.94f, 1.0f),  // Slightly lighter gray for frame background
                  ImVec4(0.0f, 0.0f, 0.0f, 1.0f),     // Black text
                  ImVec4(0.0f, 0.6f, 0.0f, 1.0f),     // Dark green text
                  ImVec4(0.8f, 0.0f, 0.0f, 1.0f)      // Dark red text
              }}};

// Global theme variables
ImVec4 button_color;
ImVec4 button_hover_color;
ImVec4 button_active_color;
ImVec4 dialog_title_color;
ImVec4 clear_color;
ImVec4 frame_bg_color;
ImVec4 text_color;
ImVec4 text_color_green;
ImVec4 text_color_red;

SDL_Texture *color_buffer_texture;
SDL_Window *window;
SDL_Renderer *renderer;

#ifdef _WIN32
#include <windows.h>

void launch_process_win(const char *path)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL,        // No module name (use command line)
                       (LPSTR)path, // Command line
                       NULL,        // Process handle not inheritable
                       NULL,        // Thread handle not inheritable
                       FALSE,       // Set handle inheritance to FALSE
                       0,           // No creation flags
                       NULL,        // Use parent's environment block
                       NULL,        // Use parent's starting directory
                       &si,         // Pointer to STARTUPINFO structure
                       &pi)         // Pointer to PROCESS_INFORMATION structure
    )
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
#endif

static void help_marker(const char *desc)
{
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void apply_theme(const std::string &theme_name)
{
    auto it = themes.find(theme_name);
    if (it != themes.end())
    {
        const Theme &theme = it->second;
        button_color = theme.button_color;
        button_hover_color = theme.button_hover_color;
        button_active_color = theme.button_active_color;
        dialog_title_color = theme.dialog_title_color;
        clear_color = theme.clear_color;
        frame_bg_color = theme.frame_bg_color;
        text_color = theme.text_color;
        text_color_green = theme.text_color_green;
        text_color_red = theme.text_color_red;
    }
}

const std::string APP_NAME = "just_launch_doom";
ImGui::FileBrowser gzdoom_file_dialog;
ImGui::FileBrowser iwad_file_dialog;
ImGui::FileBrowser pwad_file_dialog(ImGuiFileBrowserFlags_SelectDirectory);

void set_cursor_hand()
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}

std::string get_application_support_path()
{
#ifdef _WIN32 // Windows platform
    const char *appDataDir = getenv("APPDATA");
    assert(appDataDir != nullptr);

    std::string path = std::string(appDataDir) + "\\" + APP_NAME;
#elif __APPLE__ // macOS platform
    const char *homeDir = getenv("HOME");
    assert(homeDir != nullptr);

    std::string path = std::string(homeDir) + "/Library/Application Support/" + APP_NAME;
#else           // Other platforms (assuming Linux/Unix-like)
    const char *homeDir = getenv("HOME");
    assert(homeDir != nullptr);

    std::string path = std::string(homeDir) + "/." + APP_NAME;
#endif

    std::filesystem::create_directories(path);
    return path;
}

std::string get_initial_application_path()
{
    return "/Applications";
}

std::string get_config_file_path()
{
    return get_application_support_path() + "/config.json";
}

std::string get_launch_command()
{
    std::string command = config["gzdoom_filepath"];
    std::string iwad = config["iwad_filepath"];
    std::string pwad = "";
    std::string custom_params = config["custom_params"];

    // go through pwads and add them to the command
    for (size_t i = 0; i < pwads.size(); i++)
    {
        if (pwads[i].second)
        {
            pwad += "\"" + pwads[i].first + "\" ";
        }
    }

    if (!pwad.empty())
    {
        pwad = " -file " + pwad;
    }

    std::string cmd = "\"" + command + "\"" + " " + "-iwad " + "\"" + iwad + "\"" + " " + pwad;

    if (!custom_params.empty())
    {
        cmd += " " + custom_params;
    }

    return cmd;
}

bool write_config_file(const std::string &path, nlohmann::json &config)
{
    // Write to file
    std::ofstream file(path);
    if (file.is_open())
    {
        file << config.dump(4); // dump with 4 spaces indentation
        file.close();
    }
    else
    {
        return false;
    }

    return true;
}

bool read_config_file(std::string &path, nlohmann::json &config)
{
    // if file exists, load it and put put data into config
    // if not, create it and write some default settings
    std::ifstream file(path);
    if (file.is_open())
    {
        file >> config;
        file.close();
    }
    else
    {
        write_config_file(path, config);
    }

    return true;
}

void populate_pwad_list()
{
    auto selected_pwads = config["selected_pwads"];
    pwads.clear();

    // Iterate through all PWAD directories
    for (const auto &pwad_dir : config["pwad_directories"])
    {
        std::string pwadPath = pwad_dir;

        if (!std::filesystem::exists(pwadPath) || !std::filesystem::is_directory(pwadPath))
        {
            std::cerr << "The specified PWAD path does not exist or is not a directory: " << pwadPath << std::endl;
            continue;
        }

        for (const auto &entry : std::filesystem::directory_iterator(pwadPath))
        {
            if (entry.is_regular_file())
            {
                std::string filePath = entry.path().string();

                bool selected = false;
                for (const auto &selected_pwad : selected_pwads)
                {
                    if (selected_pwad == filePath)
                    {
                        selected = true;
                        break;
                    }
                }

                pwads.emplace_back(filePath, selected);
            }
        }
    }
}

void show_pwad_list()
{
    ImGui::SeparatorText("Select PWAD(s)");

    ImVec2 avail = ImGui::GetContentRegionAvail();
    // Account for custom parameters field, final command field, and launch button
    float reserved_height = 4 * ImGui::GetFrameHeightWithSpacing() + launch_button_height;
    ImVec2 listSize = ImVec2(avail.x, avail.y - reserved_height);

    if (ImGui::BeginListBox("##pwad_list_id", listSize))
    {
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 0.5f, 0.0f, 1.0f)); // Green checkmark
        ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color);                   // Theme frame background
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);                          // Theme text color

        // Make unchecked boxes visible with a border color matching the button color
        ImGui::PushStyleColor(ImGuiCol_Border, button_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        for (size_t i = 0; i < pwads.size(); i++)
        {
            // Using PushID is important for ensuring unique IDs within a loop
            ImGui::PushID(i);

            std::string pwad_file_path = pwads[i].first;

            std::filesystem::path path_obj(pwad_file_path);
            std::string filename = path_obj.filename().string();

            // Checkbox for selection. Use the label from the pair
            if (ImGui::Checkbox(filename.c_str(), &pwads[i].second))
            {
                config["selected_pwads"] = nlohmann::json::array();
                for (size_t j = 0; j < pwads.size(); j++)
                {
                    if (pwads[j].second)
                    {
                        config["selected_pwads"].push_back(pwads[j].first);
                    }
                }
                write_config_file(get_config_file_path(), config);
            }

            // Add tooltip showing full path
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(pwad_file_path.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            set_cursor_hand();

            ImGui::PopID(); // Don't forget to pop the ID after each element
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4); // Pop all colors including the border
        ImGui::EndListBox();
    }
}

void show_launch_button()
{
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5, 0.0, .75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.0f, 0.0f, .75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, .75f)); // Darker

    if (ImGui::Button("Just Launch Doom!", ImVec2(-1, launch_button_height)))
    {
        std::string cmd = get_launch_command();
        config["cmd"] = cmd;
        config["selected_pwads"] = nlohmann::json::array();

        for (size_t i = 0; i < pwads.size(); i++)
        {
            if (pwads[i].second)
            {
                config["selected_pwads"].push_back(pwads[i].first);
            }
        }

#ifdef _WIN32
        launch_process_win(cmd.c_str());
#else
        system(cmd.c_str());
#endif
    }
    ImGui::PopStyleColor(3);
    set_cursor_hand();
}

void show_gzdoom_button()
{
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, button_color);
    ;

    if (ImGui::Button("Select Doom executable"))
    {
        gzdoom_file_dialog.SetTitle("Select Doom executable");

        std::string gzdoom_filepath = config["gzdoom_filepath"];

        if (gzdoom_filepath.empty())
        {
            gzdoom_file_dialog.SetPwd(get_initial_application_path());
        }
        else
        {
            gzdoom_file_dialog.SetPwd(gzdoom_filepath);
        }

        gzdoom_file_dialog.Open();
    }
    help_marker("e.g. gzdoom, chocolate-doom, etc.");
    set_cursor_hand();

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dialog_title_color);
    gzdoom_file_dialog.Display();
    if (gzdoom_file_dialog.HasSelected())
    {
        config["gzdoom_filepath"] = gzdoom_file_dialog.GetSelected().string();
        gzdoom_file_dialog.ClearSelected();
    }
    ImGui::PopStyleColor(1);

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["gzdoom_filepath"];

    if (path.empty())
    {
        ImGui::TextColored(text_color_red, "No Doom executable selected");
    }
    else
    {
        std::filesystem::path path_obj(path);
        ImGui::TextColored(text_color_green, "%s", path_obj.filename().string().c_str());
    }

    ImGui::PopStyleColor(4);
}

void show_iwad_button()
{
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, button_color);

    std::string filepath_string = config["iwad_filepath"];

    if (ImGui::Button("Select IWAD"))
    {
        iwad_file_dialog.SetTitle("Select IWAD");

        if (filepath_string.empty())
        {
            iwad_file_dialog.SetPwd("~/");
        }
        else
        {
            std::filesystem::path file_path(filepath_string);
            std::filesystem::path directoryPath = file_path.parent_path();
            iwad_file_dialog.SetPwd(directoryPath.c_str());
        }

        iwad_file_dialog.Open();
    }
    help_marker("e.g. doom.wad, doom2.wad, heretic.wad, hexen.wad, strife1.wad, etc.");
    set_cursor_hand();

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dialog_title_color);
    iwad_file_dialog.Display();
    if (iwad_file_dialog.HasSelected())
    {
        config["iwad_filepath"] = iwad_file_dialog.GetSelected().string();
        iwad_file_dialog.ClearSelected();
    }
    ImGui::PopStyleColor(1);

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["iwad_filepath"];

    if (path.empty())
    {
        ImGui::TextColored(text_color_red, "No IWAD selected (e.g. doom.wad)");
    }
    else
    {
        std::filesystem::path path_obj(path);
        ImGui::TextColored(text_color_green, "%s", path_obj.filename().string().c_str());
    }

    ImGui::PopStyleColor(4);
}

void show_pwad_button()
{
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, button_color);

    if (ImGui::Button("Add PWAD directory"))
    {
        pwad_file_dialog.SetTitle("Select PWAD Directory");

        if (config["pwad_directories"].empty())
        {
            pwad_file_dialog.SetPwd("~/");
        }
        else
        {
            // Use the last added directory as the starting point
            std::string last_dir = config["pwad_directories"].back();
            pwad_file_dialog.SetPwd(last_dir);
        }

        pwad_file_dialog.Open();
    }
    help_marker("Add a directory containing PWADs");
    set_cursor_hand();

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dialog_title_color);
    pwad_file_dialog.Display();
    if (pwad_file_dialog.HasSelected())
    {
        std::string new_dir = pwad_file_dialog.GetSelected().string();
        // Check if directory is already in the list
        bool exists = false;
        for (const auto &dir : config["pwad_directories"])
        {
            if (dir == new_dir)
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            config["pwad_directories"].push_back(new_dir);
            populate_pwad_list();
        }
        pwad_file_dialog.ClearSelected();
    }
    ImGui::PopStyleColor(1);

    // Display list of PWAD directories
    if (!config["pwad_directories"].empty())
    {
        ImGui::SeparatorText("PWAD Directories");
        for (size_t i = 0; i < config["pwad_directories"].size(); i++)
        {
            ImGui::PushID(i);
            std::string dir = config["pwad_directories"][i];
            ImGui::TextColored(text_color_green, "%s", dir.c_str());
            ImGui::SameLine();
            if (ImGui::Button("Remove"))
            {
                config["pwad_directories"].erase(config["pwad_directories"].begin() + i);
                populate_pwad_list();
            }
            set_cursor_hand();
            ImGui::PopID();
        }
    }
    else
    {
        ImGui::TextColored(text_color_red, "No PWAD directories added");
    }

    ImGui::PopStyleColor(4);
}

void show_command()
{
    ImGui::SeparatorText("Custom Parameters");
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    // Copy current custom params to buffer if it exists
    if (!config["custom_params"].empty())
    {
        strncpy(custom_params_buf, config["custom_params"].get<std::string>().c_str(), sizeof(custom_params_buf) - 1);
        custom_params_buf[sizeof(custom_params_buf) - 1] = '\0';
    }

    if (ImGui::InputText("##custom_params_id", custom_params_buf, sizeof(custom_params_buf)))
    {
        // Update config with new value
        config["custom_params"] = custom_params_buf;
    }

    ImGui::PopStyleColor(2);

    ImGui::SeparatorText("Final Command");
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color);
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    snprintf(command_buf, IM_ARRAYSIZE(command_buf), "%s", get_launch_command().c_str());
    ImGui::InputText("##command_id", command_buf, IM_ARRAYSIZE(command_buf), ImGuiInputTextFlags_ReadOnly);

    ImGui::PopStyleColor(2);
}

void show_theme_selector()
{
    ImGui::PushStyleColor(ImGuiCol_Button, frame_bg_color); // Use frame background color for main button
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);        // Theme text color
    ImGui::PushStyleColor(ImGuiCol_PopupBg, frame_bg_color); // Theme background for popup
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color); // Background of the combo box
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, frame_bg_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, frame_bg_color);

    // Set the arrow color to match the button color in light theme
    if (config["theme"] == "light")
    {
        ImGui::PushStyleColor(ImGuiCol_Text, button_color);
    }

    std::string theme_label = "Theme: " + config["theme"].get<std::string>();
    theme_label[0] = std::toupper(theme_label[0]); // Capitalize first letter
    theme_label[7] = std::toupper(theme_label[7]); // Capitalize first letter of theme name

    // Set a fixed width for the combo box
    ImGui::PushItemWidth(120);
    if (ImGui::BeginCombo("##Theme", theme_label.c_str()))
    {
        // Pop the arrow color before showing the items
        if (config["theme"] == "light")
        {
            ImGui::PopStyleColor();
        }

        for (const auto &theme : themes)
        {
            bool is_selected = (config["theme"] == theme.first);
            std::string display_name = theme.first;
            display_name[0] = std::toupper(display_name[0]); // Capitalize first letter

            if (ImGui::Selectable(display_name.c_str(), is_selected))
            {
                config["theme"] = theme.first;
                apply_theme(theme.first);
                write_config_file(get_config_file_path(), config);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    else if (config["theme"] == "light")
    {
        ImGui::PopStyleColor(); // Pop the arrow color if combo isn't open
    }
    ImGui::PopItemWidth();
    set_cursor_hand();

    ImGui::PopStyleColor(8);
}

void show_ui()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    // Set global text color for the window
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color);

    // Put everything in a full screen window
    if (ImGui::Begin("fullscreen window", nullptr, window_flags))
    {
        // Calculate positions for right-aligned elements
        float window_width = ImGui::GetWindowWidth();
        float theme_width = 120; // Fixed width for theme selector
        float spacing = 10;      // Space between elements

        // Position theme selector in the top right
        ImGui::SetCursorPos(ImVec2(window_width - theme_width - spacing, spacing));
        show_theme_selector();

        // Reset cursor for consistent left alignment of all buttons
        ImGui::SetCursorPos(ImVec2(spacing, spacing));
        show_gzdoom_button();

        // Move cursor down for next row
        ImGui::SetCursorPos(ImVec2(spacing, ImGui::GetCursorPosY() + spacing));
        show_iwad_button();
        show_pwad_button();
        show_pwad_list();
        show_command();
        show_launch_button();
    }
    ImGui::PopStyleColor(2);
}

void set_color_buffer_size()
{
    const int MAX_WIDTH = 640;
    const int MAX_HEIGHT = 480;
    float aspect_ratio_x = (float)renderer_width / (float)renderer_height;
    float aspect_ratio_y = (float)renderer_height / (float)renderer_width;

    if (aspect_ratio_x > aspect_ratio_y)
    {
        color_buffer_width = MAX_WIDTH;
        color_buffer_height = MAX_WIDTH * aspect_ratio_y;
    }
    else
    {
        color_buffer_height = MAX_HEIGHT;
        color_buffer_width = MAX_HEIGHT * aspect_ratio_x;
    }
}

void configure_color_buffer()
{
    set_color_buffer_size();

    color_buffer = new uint32_t[sizeof(uint32_t) * color_buffer_width * color_buffer_height];
    color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                             color_buffer_width, color_buffer_height);
}

void update()
{
    ImGuiIO &io = ImGui::GetIO();
    bool done = false;

    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type)
            {
            case SDL_QUIT:
                done = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                    done = true;
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    configure_color_buffer();
                    // Update the configuration with the new size
                    int newWidth = event.window.data1;
                    int newHeight = event.window.data2;
                    config["resolution"] = {newWidth, newHeight};
                    write_config_file(get_config_file_path(), config); // Optionally save to file immediately
                }
                break;
            }
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowBgAlpha(0.0f);

        ImVec2 screenSize = io.DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(screenSize);

        show_ui();

        ImGui::End();

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255),
                               (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);

        // Only show fire animation in fire theme
        if (config["theme"] == "fire")
        {
            draw_fire(color_buffer_texture, color_buffer, color_buffer_width, color_buffer_height);
            SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
        }

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }
}

void setup_config_file()
{
    std::string config_file_path = get_config_file_path();
    bool loaded = read_config_file(config_file_path, config);
    assert(loaded == true);

    if (config["gzdoom_filepath"].empty())
    {
        config["gzdoom_filepath"] = "";
    }

    if (config["iwad_path"].empty())
    {
        config["iwad_path"] = "";
    }

    if (config["selected_pwads"].empty())
    {
        config["selected_pwads"] = nlohmann::json::array();
    }

    if (config["resolution"].empty())
    {
        config["resolution"] = {800, 600};
    }

    if (config["custom_params"].empty())
    {
        config["custom_params"] = "";
    }

    // Ensure theme field exists with default value
    if (!config.contains("theme") || config["theme"].is_null())
    {
        config["theme"] = "fire";
    }
}

int setup()
{

    SDL_SetMainReady();

    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Create window with SDL_Renderer graphics context
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    std::string title;
    title += "Just Launch Doom v";
    title += VERSION;

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              config["resolution"][0], config["resolution"][1], window_flags);

    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr)
    {
        SDL_Log("Error creating SDL_Renderer!");
        return 0;
    }

    SDL_GetRendererOutputSize(renderer, &renderer_width, &renderer_height);

    configure_color_buffer();
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io.IniFilename = NULL;                                // Disable imgui.ini

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    io.FontGlobalScale = 1.0;

    setup_config_file();
    populate_pwad_list();

    // Apply initial theme
    apply_theme(config["theme"].get<std::string>());

    return 0;
}

void clean_up()
{
    bool written = write_config_file(get_config_file_path(), config);
    assert(written == true);

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int, char **)
{
    setup();
    update();
    clean_up();

    return 0;
}