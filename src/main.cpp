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
#include "config_migration.h"
#include "config_utils.h"
#include "launch_utils.h"

#include "fire.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

#define VERSION "0.1.14"

const std::vector<std::string> CONFIG_EXTENSIONS = {".cfg", ".ini"};

#ifdef _WIN32
const std::vector<std::string> EXECUTABLE_EXTENSIONS = {".exe", ".bat", ".cmd", ".ps1"};
#elif __APPLE__
const std::vector<std::string> EXECUTABLE_EXTENSIONS = {"", ".sh"};
#else
const std::vector<std::string> EXECUTABLE_EXTENSIONS = {"", ".sh", ".appimage"};
#endif

int renderer_width, renderer_height;
int color_buffer_width, color_buffer_height;
int launch_button_height = 35;
char command_buf[1024] = "THIS IS THE COMMAND";
char custom_params_buf[1024] = "";
std::vector<PwadFileInfo> pwads;

// Global variables for TXT file error messaging
std::string txt_file_error_message = "";

uint32_t *color_buffer = nullptr;
nlohmann::json config = {
    {"resolution", {800, 600}},
    {"pwad_directories", nlohmann::json::array()},
    {"iwad_filepath", ""},
    {"iwads", nlohmann::json::array()},
    {"selected_iwad", ""},
    {"selected_pwads", nlohmann::json::array()},
    {"custom_params", ""},
    {"theme", "fire"},
    {"config_files", nlohmann::json::array()},
    {"selected_config", ""},
    {"font_size", 1.0f},
    {"sdl_renderer", "auto"},
    {"sdl_renderer_inherit", false}};

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
                  ImVec4(0.45f, 0.45f, 0.45f, 1.0f),  // Dark gray for buttons
                  ImVec4(0.55f, 0.55f, 0.55f, 1.0f),  // Slightly lighter gray for hover
                  ImVec4(0.35f, 0.35f, 0.35f, 1.0f),  // Darker gray for active
                  ImVec4(0.0f, 0.0f, 0.0f, 1.0f),     // Black for dialog title
                  ImVec4(0.85f, 0.85f, 0.85f, 1.00f), // Light gray for background
                  ImVec4(0.80f, 0.80f, 0.80f, 1.0f),  // Slightly darker gray for frame background
                  ImVec4(0.0f, 0.0f, 0.0f, 1.0f),     // Black text
                  ImVec4(0.0f, 0.4f, 0.0f, 1.0f),     // Darker green text
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

bool show_settings = false;               // Global state variable to track the visibility of the settings view
bool pin_selected_pwads_to_top = true;       // Global variable to track the pinning behavior
bool group_pwads_by_directory = true;        // Global variable to track directory grouping
static int selected_font_scale_index = 1; // Default to 1.0f (100%)

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
ImGui::FileBrowser gzdoom_file_dialog(ImGuiFileBrowserFlags_CloseOnEsc);
ImGui::FileBrowser iwad_file_dialog(ImGuiFileBrowserFlags_CloseOnEsc);
ImGui::FileBrowser pwad_file_dialog(ImGuiFileBrowserFlags_SelectDirectory);
ImGui::FileBrowser config_file_dialog(ImGuiFileBrowserFlags_CloseOnEsc);

void set_cursor_hand()
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}

std::string get_initial_application_path()
{
    return "/Applications";
}

// Function to open text files cross-platform with error handling
bool open_text_file(const std::string &filepath)
{
    txt_file_error_message = ""; // Clear any previous error

#ifdef _WIN32
    if (system(("start \"\" \"" + filepath + "\"").c_str()) == 0)
    {
        return true;
    }
    txt_file_error_message = "Failed to open text file with Windows default application";
    return false;

#elif __APPLE__
    if (system(("open \"" + filepath + "\"").c_str()) == 0)
    {
        return true;
    }
    txt_file_error_message = "Failed to open text file with macOS default application";
    return false;

#else // Linux
      // 1. Check if we're in WSL and wslview is available
    if (system("command -v wslview >/dev/null 2>&1") == 0)
    {
        if (system(("wslview \"" + filepath + "\"").c_str()) == 0)
        {
            return true;
        }
    }

    // 2. Try xdg-open
    if (system("command -v xdg-open >/dev/null 2>&1") == 0)
    {
        if (system(("xdg-open \"" + filepath + "\"").c_str()) == 0)
        {
            return true;
        }
    }

    // 3. Fallback to common text editors
    const std::vector<std::string> editors = {"gedit", "kate", "mousepad", "nano"};
    for (const auto &editor : editors)
    {
        if (system(("command -v " + editor + " >/dev/null 2>&1").c_str()) == 0)
        {
            if (system((editor + " \"" + filepath + "\" &").c_str()) == 0)
            {
                return true;
            }
        }
    }

    // Set error message for Linux
    txt_file_error_message = "No text editor found. Install: gedit, kate, mousepad, or nano";
    return false;
#endif
}

// Display dismissible error message for text file operations
void show_txt_file_error()
{
    if (!txt_file_error_message.empty())
    {
        ImVec4 text_color_red = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Match existing red color

        ImGui::TextColored(text_color_red, "%s", txt_file_error_message.c_str());
        ImGui::SameLine();

        // Small dismiss button following existing button patterns
        if (ImGui::Button("OK", ImVec2(40, 0)))
        {
            txt_file_error_message = "";
        }
    }
}

std::string get_launch_command()
{
    std::string command = "\"" + config["selected_executable"].get<std::string>() + "\"";
    std::string iwad = config["selected_iwad"];
    std::string custom_params = config["custom_params"];
    std::string selected_config = config["selected_config"];

    command += build_launch_file_args(pwads);

    if (!iwad.empty())
    {
        command += " -iwad \"" + iwad + "\"";
    }

    if (!custom_params.empty())
    {
        command += " " + custom_params;
    }

    if (!selected_config.empty())
    {
        command += " -config \"" + selected_config + "\"";
    }

    return command;
}

void populate_pwad_list()
{
    pwads.clear();

    // Helper function to check if a file has an allowed extension (WAD, DEH, or EDF)
    auto has_allowed_extension = [](const std::string &filename)
    {
        std::string lower_filename = filename;
        std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
        auto matches_ext = [&lower_filename](const std::vector<std::string> &extensions)
        {
            return std::any_of(extensions.begin(), extensions.end(),
                               [&lower_filename](const std::string &ext)
                               {
                                   return lower_filename.length() >= ext.length() &&
                                          lower_filename.substr(lower_filename.length() - ext.length()) == ext;
                               });
        };
        return matches_ext(WAD_EXTENSIONS) || matches_ext(DEH_EXTENSIONS) || matches_ext(EDF_EXTENSIONS);
    };

    // go through each directory in pwad_directories
    for (const auto &dir : config["pwad_directories"])
    {
        std::filesystem::path directory_path(dir);
        if (std::filesystem::exists(directory_path) && std::filesystem::is_directory(directory_path))
        {
            for (const auto &entry : std::filesystem::directory_iterator(directory_path))
            {
                if (entry.is_regular_file() && has_allowed_extension(entry.path().filename().string()))
                {
                    std::string file_path = entry.path().string();
                    bool is_selected = false;

                    // Check if this file is in the selected_pwads array
                    for (const auto &selected_pwad : config["selected_pwads"])
                    {
                        if (selected_pwad == file_path)
                        {
                            is_selected = true;
                            break;
                        }
                    }

                    // Check for companion .txt file
                    std::string txt_file_path = "";
                    std::filesystem::path pwad_path(file_path);
                    std::string base_name = pwad_path.stem().string(); // Get filename without extension
                    std::string txt_path = (pwad_path.parent_path() / (base_name + ".txt")).string();

                    if (std::filesystem::exists(txt_path))
                    {
                        txt_file_path = txt_path;
                    }

                    pwads.push_back({file_path, is_selected, txt_file_path, dir.get<std::string>()});
                }
            }
        }
    }

    // Build a map from directory path to its index in config for stable ordering
    std::map<std::string, int> dir_order;
    for (size_t i = 0; i < config["pwad_directories"].size(); i++)
    {
        dir_order[config["pwad_directories"][i].get<std::string>()] = i;
    }

    // Sort the pwads by selection status first (if pinning), then by directory (if grouping), then by filename
    std::sort(pwads.begin(), pwads.end(),
              [&dir_order](const auto &a, const auto &b)
              {
                  // Pin selected PWADs to top if enabled
                  if (pin_selected_pwads_to_top && a.selected != b.selected)
                  {
                      return a.selected > b.selected;
                  }

                  // Group by directory if enabled
                  if (group_pwads_by_directory && a.directory != b.directory)
                  {
                      int a_order = dir_order.count(a.directory) ? dir_order[a.directory] : 9999;
                      int b_order = dir_order.count(b.directory) ? dir_order[b.directory] : 9999;
                      return a_order < b_order;
                  }

                  // Sort alphabetically by filename
                  std::string a_name = std::filesystem::path(a.filepath).filename().string();
                  std::string b_name = std::filesystem::path(b.filepath).filename().string();
                  std::transform(a_name.begin(), a_name.end(), a_name.begin(), ::tolower);
                  std::transform(b_name.begin(), b_name.end(), b_name.begin(), ::tolower);
                  return a_name < b_name;
              });
}

void show_pwad_list()
{
    ImGui::SeparatorText("Select PWAD(s)");

    // Add a 'Reload' button next to the label with theme colors
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    if (ImGui::Button("Reload"))
    {
        populate_pwad_list(); // Recheck for files in the PWAD directories
    }
    set_cursor_hand(); // Set cursor to hand when hovering
    ImGui::PopStyleColor(3);

    // Add a search bar next to the 'Reload' button
    static char search_buf[128] = "";
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Border, button_color);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
    ImGui::PushItemWidth(200);
    ImGui::InputTextWithHint("##search_pwad", "Search PWADs...", search_buf, sizeof(search_buf));
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float reserved_height = 4 * ImGui::GetFrameHeightWithSpacing() + launch_button_height;
    ImVec2 listSize = ImVec2(avail.x, avail.y - reserved_height);

    if (ImGui::BeginListBox("##pwad_list_id", listSize))
    {
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 0.5f, 0.0f, 1.0f)); // Green checkmark
        ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color);                   // Theme frame background
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);                          // Theme text color
        ImGui::PushStyleColor(ImGuiCol_Border, button_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        // Track current directory for rendering headers
        std::string current_directory = "";
        bool show_directory_headers = group_pwads_by_directory && config["pwad_directories"].size() > 1;
        bool current_directory_collapsed = false;

        for (size_t i = 0; i < pwads.size(); i++)
        {
            std::string pwad_file_path = pwads[i].filepath;
            std::string filename = std::filesystem::path(pwad_file_path).filename().string();

            // Perform case-insensitive search filtering
            std::string lower_filename = filename;
            std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
            std::string lower_search = search_buf;
            std::transform(lower_search.begin(), lower_search.end(), lower_search.begin(), ::tolower);

            if (!lower_search.empty() && lower_filename.find(lower_search) == std::string::npos)
            {
                continue; // Skip items that don't match the search
            }

            // Render collapsible directory header when directory changes (skip for pinned selected items)
            if (show_directory_headers && !(pin_selected_pwads_to_top && pwads[i].selected))
            {
                if (pwads[i].directory != current_directory)
                {
                    current_directory = pwads[i].directory;
                    std::string dir_name = std::filesystem::path(current_directory).filename().string();
                    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
                    current_directory_collapsed = !ImGui::CollapsingHeader(dir_name.c_str());
                }
                if (current_directory_collapsed)
                {
                    continue;
                }
            }

            ImGui::PushID(i);
            if (ImGui::Checkbox(filename.c_str(), &pwads[i].selected))
            {
                config["selected_pwads"] = nlohmann::json::array();
                for (size_t j = 0; j < pwads.size(); j++)
                {
                    if (pwads[j].selected)
                    {
                        config["selected_pwads"].push_back(pwads[j].filepath);
                    }
                }
                write_config_file(get_config_file_path(), config);
                populate_pwad_list(); // Re-sort and update the list after selection change
            }

            // Add TXT button if companion text file exists
            if (!pwads[i].txt_filepath.empty())
            {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

                if (ImGui::Button("TXT", ImVec2(35, 0)))
                {
                    open_text_file(pwads[i].txt_filepath);
                }

                set_cursor_hand();
                ImGui::PopStyleColor(4);

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Open companion text file:");
                    ImGui::TextUnformatted(pwads[i].txt_filepath.c_str());
                    ImGui::EndTooltip();
                }
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::TextUnformatted(pwad_file_path.c_str());
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            set_cursor_hand();
            ImGui::PopID();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(4);
        ImGui::EndListBox();
    }

    // Display any text file error messages
    show_txt_file_error();
}

void show_launch_button()
{
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.5, 0.0, .75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.0f, 0.0f, .75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.0f, 0.0f, .75f)); // Darker

    // Disable button if no executable is selected
    bool has_executable = !config["selected_executable"].empty();
    if (!has_executable)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, .75f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    }

    if (ImGui::Button("Just Launch Doom!", ImVec2(-1, launch_button_height)) && has_executable)
    {
        std::string cmd = get_launch_command();
        config["cmd"] = cmd;
        config["selected_pwads"] = nlohmann::json::array();

        for (size_t i = 0; i < pwads.size(); i++)
        {
            if (pwads[i].selected)
            {
                config["selected_pwads"].push_back(pwads[i].filepath);
            }
        }

#ifdef _WIN32
        launch_process_win(cmd.c_str());
#else
        system(cmd.c_str());
#endif
    }

    if (!has_executable)
    {
        ImGui::PopStyleColor(2);
    }
    ImGui::PopStyleColor(3);
    set_cursor_hand();
}

void show_executable_selector()
{
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);        // Theme text color
    ImGui::PushStyleColor(ImGuiCol_PopupBg, frame_bg_color); // Theme background for popup
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color); // Background of the combo box
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, frame_bg_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, frame_bg_color);

    // Executable selection dropdown
    std::string selected_exec_path = config["selected_executable"];
    std::string selected_exec_name = selected_exec_path.empty() ? "Executable: None" : "Executable: " + std::filesystem::path(selected_exec_path).filename().string();
    const char *display_text = selected_exec_name.c_str();

    // Set a maximum width for the combo box
    ImGui::PushItemWidth(300); // Maximum width of 300 pixels

    if (ImGui::BeginCombo("##exec_select", display_text, ImGuiComboFlags_WidthFitPreview))
    {
        // Only show "None" option if there are no executables
        if (config["doom_executables"].empty())
        {
            bool is_none_selected = config["selected_executable"].empty();
            if (ImGui::Selectable("Executable: None", is_none_selected))
            {
                config["selected_executable"] = "";
                write_config_file(get_config_file_path(), config);
            }
            if (is_none_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        for (size_t i = 0; i < config["doom_executables"].size(); i++)
        {
            std::string exec_path = config["doom_executables"][i];
            std::string filename = std::filesystem::path(exec_path).filename().string();
            bool is_selected = (config["selected_executable"] == exec_path);

            if (ImGui::Selectable(("Executable: " + filename).c_str(), is_selected))
            {
                config["selected_executable"] = exec_path;
                write_config_file(get_config_file_path(), config);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    set_cursor_hand(); // Add hand cursor for dropdown
    ImGui::PopItemWidth();

    // Add Executable button
    ImGui::SameLine();
    if (ImGui::Button("Add##exe"))
    {
        gzdoom_file_dialog.SetTitle("Select Doom Executable");

        // Set the initial directory based on the current executable if it exists
        if (!config["selected_executable"].empty())
        {
            std::filesystem::path current_path(config["selected_executable"]);
            gzdoom_file_dialog.SetPwd(current_path.parent_path().string());
        }
        else
        {
            gzdoom_file_dialog.SetPwd(get_initial_application_path());
        }

        gzdoom_file_dialog.Open();
    }
    help_marker("Add a new Doom executable to the list");
    set_cursor_hand();

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dialog_title_color);
    gzdoom_file_dialog.Display();
    if (gzdoom_file_dialog.HasSelected())
    {
        std::string new_exec = gzdoom_file_dialog.GetSelected().string();
        // Check if executable is already in the list
        bool exists = false;
        for (const auto &exec_path : config["doom_executables"])
        {
            if (exec_path == new_exec)
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            config["doom_executables"].push_back(new_exec);
            // Always select the newly added executable
            config["selected_executable"] = new_exec;
            write_config_file(get_config_file_path(), config);
        }
        gzdoom_file_dialog.ClearSelected();
        // Sort the doom_executables list alphabetically after adding a new executable
        std::sort(config["doom_executables"].begin(), config["doom_executables"].end());
    }
    ImGui::PopStyleColor(1);

    // Remove button
    if (!config["selected_executable"].empty())
    {
        ImGui::SameLine();
        if (ImGui::Button("Remove##exe"))
        {
            // Find and remove the selected executable
            for (size_t i = 0; i < config["doom_executables"].size(); i++)
            {
                if (config["doom_executables"][i] == config["selected_executable"])
                {
                    config["doom_executables"].erase(config["doom_executables"].begin() + i);
                    // If this was the last executable, clear selection
                    if (config["doom_executables"].empty())
                    {
                        config["selected_executable"] = "";
                    }
                    // Otherwise select the first available executable
                    else
                    {
                        config["selected_executable"] = config["doom_executables"][0];
                    }
                    write_config_file(get_config_file_path(), config);
                    break;
                }
            }
        }
        set_cursor_hand();
    }

    ImGui::PopStyleColor(8);
}

void show_iwad_button()
{
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);        // Theme text color
    ImGui::PushStyleColor(ImGuiCol_PopupBg, frame_bg_color); // Theme background for popup
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color); // Background of the combo box
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, frame_bg_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, frame_bg_color);

    std::string filepath_string = config["selected_iwad"];
    std::string selected_iwad_name = filepath_string.empty() ? "IWAD: None" : "IWAD: " + std::filesystem::path(filepath_string).filename().string();

    ImGui::PushItemWidth(300);
    if (ImGui::BeginCombo("##iwad_selector", selected_iwad_name.c_str(), ImGuiComboFlags_WidthFitPreview))
    {
        // Only show "None" option if there are no IWADs
        if (config["iwads"].empty())
        {
            bool is_none_selected = config["selected_iwad"].empty();
            if (ImGui::Selectable("IWAD: None", is_none_selected))
            {
                config["selected_iwad"] = "";
                write_config_file(get_config_file_path(), config);
            }
            if (is_none_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        for (size_t i = 0; i < config["iwads"].size(); i++)
        {
            std::string iwad_path = config["iwads"][i];
            std::string filename = std::filesystem::path(iwad_path).filename().string();
            bool is_selected = (config["selected_iwad"] == iwad_path);
            if (ImGui::Selectable(("IWAD: " + filename).c_str(), is_selected))
            {
                config["selected_iwad"] = iwad_path;
                write_config_file(get_config_file_path(), config);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    set_cursor_hand(); // Add hand cursor for dropdown
    ImGui::PopItemWidth();

    ImGui::SameLine();

    if (ImGui::Button("Add##iwad"))
    {

        // Set the initial directory based on the current IWAD if it exists
        if (!config["selected_iwad"].empty())
        {
            std::filesystem::path current_path(config["selected_iwad"]);
            iwad_file_dialog.SetPwd(current_path.parent_path().string());
        }
        else
        {
#ifdef __APPLE__
            iwad_file_dialog.SetPwd("/Applications");
#else
            iwad_file_dialog.SetPwd(std::filesystem::current_path().string());
#endif
        }

        ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dialog_title_color);
        iwad_file_dialog.Open();
        ImGui::PopStyleColor(1);
    }
    help_marker("Add a new IWAD to the list");
    set_cursor_hand();

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dialog_title_color);
    iwad_file_dialog.Display();
    if (iwad_file_dialog.HasSelected())
    {
        std::string new_iwad = iwad_file_dialog.GetSelected().string();
        bool found = false;
        for (const auto &iwad : config["iwads"])
        {
            if (iwad.get<std::string>() == new_iwad)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            config["iwads"].push_back(new_iwad);
            config["selected_iwad"] = new_iwad;
            write_config_file(get_config_file_path(), config);
            // Sort the IWAD list alphabetically after adding a new IWAD
            std::sort(config["iwads"].begin(), config["iwads"].end());
        }
        iwad_file_dialog.ClearSelected();
    }
    if (!config["selected_iwad"].empty())
    {
        ImGui::SameLine();
        if (ImGui::Button("Remove##iwad"))
        {
            std::string current_iwad = config["selected_iwad"];
            for (size_t i = 0; i < config["iwads"].size(); i++)
            {
                if (config["iwads"][i] == current_iwad)
                {
                    config["iwads"].erase(config["iwads"].begin() + i);
                    if (config["iwads"].empty())
                    {
                        config["selected_iwad"] = "";
                    }
                    else
                    {
                        config["selected_iwad"] = config["iwads"][0];
                    }
                    write_config_file(get_config_file_path(), config);
                    break;
                }
            }
        }
        set_cursor_hand();
    }

    ImGui::PopStyleColor(1);
    ImGui::PopStyleColor(8);
}

void show_config_button()
{
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);        // Theme text color
    ImGui::PushStyleColor(ImGuiCol_PopupBg, frame_bg_color); // Theme background for popup
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color); // Background of the combo box
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, frame_bg_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, frame_bg_color);

    // Config selection dropdown first
    // Store the strings in stable variables to avoid dangling pointers
    std::string selected_config_path = config["selected_config"];
    std::string selected_config_name = selected_config_path.empty() ? "Config file: None" : "Config file: " + std::filesystem::path(selected_config_path).filename().string();
    const char *display_text = selected_config_name.c_str();

    // Set a maximum width for the combo box
    ImGui::PushItemWidth(300); // Maximum width of 300 pixels

    if (ImGui::BeginCombo("##config_select", display_text, ImGuiComboFlags_WidthFitPreview))
    {
        // Add "None" option at the top
        bool is_none_selected = config["selected_config"].empty();
        if (ImGui::Selectable("Config file: None", is_none_selected))
        {
            config["selected_config"] = "";
            write_config_file(get_config_file_path(), config);
        }
        if (is_none_selected)
        {
            ImGui::SetItemDefaultFocus();
        }

        // Add separator after None option if we have configs
        if (!config["config_files"].empty())
        {
            ImGui::Separator();
        }

        for (size_t i = 0; i < config["config_files"].size(); i++)
        {
            std::string config_path = config["config_files"][i];
            std::string filename = std::filesystem::path(config_path).filename().string();
            bool is_selected = (config["selected_config"] == config_path);

            if (ImGui::Selectable(("Config file: " + filename).c_str(), is_selected))
            {
                config["selected_config"] = config_path;
                write_config_file(get_config_file_path(), config);
            }
            if (is_selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    set_cursor_hand(); // Add hand cursor for dropdown
    ImGui::PopItemWidth();

    // Add Config button
    ImGui::SameLine();
    if (ImGui::Button("Add##config"))
    {
        config_file_dialog.SetTitle("Select Config File");

        if (config["config_files"].empty())
        {
            config_file_dialog.SetPwd("~/");
        }
        else
        {
            // Use the last added config's directory as starting point
            std::string last_config = config["config_files"].back();
            std::filesystem::path file_path(last_config);
            std::filesystem::path directoryPath = file_path.parent_path();
            config_file_dialog.SetPwd(directoryPath.c_str());
        }

        config_file_dialog.Open();
    }
    help_marker("Add a new config file to the list");
    set_cursor_hand();

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, dialog_title_color);
    config_file_dialog.Display();
    if (config_file_dialog.HasSelected())
    {
        std::string new_config = config_file_dialog.GetSelected().string();
        // Check if config is already in the list
        bool exists = false;
        for (const auto &config_path : config["config_files"])
        {
            if (config_path == new_config)
            {
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            config["config_files"].push_back(new_config);
            config["selected_config"] = new_config; // Automatically select the new config
            write_config_file(get_config_file_path(), config);
        }
        config_file_dialog.ClearSelected();
    }
    ImGui::PopStyleColor(1);

    // Remove button
    if (!config["selected_config"].empty())
    {
        ImGui::SameLine();
        if (ImGui::Button("Remove##config"))
        {
            // Find and remove the selected config
            for (size_t i = 0; i < config["config_files"].size(); i++)
            {
                if (config["config_files"][i] == config["selected_config"])
                {
                    config["config_files"].erase(config["config_files"].begin() + i);
                    config["selected_config"] = "";
                    write_config_file(get_config_file_path(), config);
                    break;
                }
            }
        }
        set_cursor_hand();
    }

    ImGui::PopStyleColor(8);
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

void show_settings_view()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, frame_bg_color); // Apply theme background color
    ImGui::PushStyleColor(ImGuiCol_Text, text_color);         // Apply theme text color
    ImGui::PushStyleColor(ImGuiCol_Button, button_color);     // Apply theme button color
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, frame_bg_color); // Apply theme frame background color

    if (ImGui::Begin("Settings", nullptr, window_flags))
    {
        ImGui::Text("Select Theme:");
        ImGui::PushItemWidth(120);
        if (ImGui::BeginCombo("##Theme", config["theme"].get<std::string>().c_str()))
        {
            for (const auto &theme : themes)
            {
                bool is_selected = (config["theme"] == theme.first);
                if (ImGui::Selectable(theme.first.c_str(), is_selected))
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
        set_cursor_hand(); // Add hand cursor for dropdown
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Spacing();

        // Add a dropdown for font scale selection
        // add font scales between 10% and 200%
        static const std::vector<float> font_scales = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f,
                                                       1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f,
                                                       1.8f, 1.9f, 2.0f};

        ImGui::Text("Font Scale:");
        ImGui::PushItemWidth(120);
        if (ImGui::BeginCombo("##FontScale", (std::to_string(static_cast<int>(font_scales[selected_font_scale_index] * 100)) + "%").c_str()))
        {
            for (int i = 0; i < font_scales.size(); ++i)
            {
                bool is_selected = (i == selected_font_scale_index);
                std::string label = std::to_string(static_cast<int>(font_scales[i] * 100)) + "%";
                if (ImGui::Selectable(label.c_str(), is_selected))
                {
                    selected_font_scale_index = i;
                    ImGui::GetIO().FontGlobalScale = font_scales[i]; // Apply the font scale
                    config["font_scale"] = font_scales[i];
                    write_config_file(get_config_file_path(), config);
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        set_cursor_hand(); // Add hand cursor for dropdown
        ImGui::PopItemWidth();

        ImGui::Spacing();

        // Add a checkbox for pinning selected PWADs to the top with a border
        ImGui::PushStyleColor(ImGuiCol_Border, button_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Checkbox("Pin Selected PWADs to Top", &pin_selected_pwads_to_top))
        {
            config["pin_selected_pwads_to_top"] = pin_selected_pwads_to_top;
            write_config_file(get_config_file_path(), config);
            populate_pwad_list(); // Resort the PWAD list based on the new checkbox value
        }
        set_cursor_hand(); // Add hand cursor for checkbox
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Add a checkbox for grouping PWADs by directory
        ImGui::PushStyleColor(ImGuiCol_Border, button_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Checkbox("Group PWADs by Directory", &group_pwads_by_directory))
        {
            config["group_pwads_by_directory"] = group_pwads_by_directory;
            write_config_file(get_config_file_path(), config);
            populate_pwad_list(); // Resort the PWAD list based on the new checkbox value
        }
        set_cursor_hand(); // Add hand cursor for checkbox
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Spacing();

        // Explanation section for SDL renderer settings
        ImGui::TextColored(text_color_green, "Renderer Conflict Fix");
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextWrapped("Some Doom ports (Woof, Chocolate Doom, DSDA-Doom) may cause crashes "
                               "when using fullscreen mode. This happens when the launcher and "
                               "the game use different SDL renderers.");
            ImGui::Spacing();
            ImGui::TextWrapped("To fix: Select 'OpenGL' or 'Direct3D11' below, then enable "
                               "'Apply renderer to launched games'. This forces both the launcher "
                               "and games to use the same renderer.");
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
        ImGui::Separator();
        
        // Show restart warning for renderer settings
        ImGui::TextColored(text_color_red, "Restart required for renderer changes to take effect");
        ImGui::Spacing();

        // Get current renderer settings for the UI controls
        std::string current_renderer = config["sdl_renderer"].get<std::string>();
        bool inherit_renderer = config["sdl_renderer_inherit"].get<bool>();

        // Add SDL Renderer dropdown (fix for issue #16)
        ImGui::Text("SDL Renderer:");
        ImGui::PushItemWidth(160);

        // Get available SDL renderers dynamically
        std::vector<std::pair<std::string, std::string>> renderer_options;
        renderer_options.push_back({"auto", "Auto (Default)"});

        // Query SDL for available render drivers
        int num_drivers = SDL_GetNumRenderDrivers();
        for (int i = 0; i < num_drivers; i++)
        {
            SDL_RendererInfo info;
            if (SDL_GetRenderDriverInfo(i, &info) == 0)
            {
                std::string driver_name = info.name;
                std::string display_name = driver_name;
                // Capitalize first letter for display
                if (!display_name.empty())
                {
                    display_name[0] = std::toupper(display_name[0]);
                }
                renderer_options.push_back({driver_name, display_name});
            }
        }

        int current_index = 0;
        for (size_t i = 0; i < renderer_options.size(); i++)
        {
            if (current_renderer == renderer_options[i].first)
            {
                current_index = i;
                break;
            }
        }

        if (ImGui::BeginCombo("##SDLRenderer", renderer_options[current_index].second.c_str()))
        {
            for (size_t i = 0; i < renderer_options.size(); i++)
            {
                bool is_selected = (current_index == i);
                if (ImGui::Selectable(renderer_options[i].second.c_str(), is_selected))
                {
                    config["sdl_renderer"] = renderer_options[i].first;
                    write_config_file(get_config_file_path(), config);
                }
                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        set_cursor_hand(); // Add hand cursor for dropdown
        ImGui::PopItemWidth();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Changes the SDL renderer used by the launcher.");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        // Add checkbox for SDL renderer inheritance (fix for issue #16)
        ImGui::PushStyleColor(ImGuiCol_Border, button_color);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        if (ImGui::Checkbox("Apply renderer to launched games", &inherit_renderer))
        {
            config["sdl_renderer_inherit"] = inherit_renderer;
            write_config_file(get_config_file_path(), config);
        }
        set_cursor_hand(); // Add hand cursor for checkbox
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("Forces launched Doom ports to use the same renderer.");
            ImGui::Text("This is the main fix for renderer conflicts.");
            ImGui::EndTooltip();
        }

        ImGui::Spacing();

        // Add an 'Okay' button to return to the main view
        if (ImGui::Button("OK", ImVec2(100, 30)))
        {
            show_settings = false;
        }
        set_cursor_hand(); // Add hand cursor for button
    }
    ImGui::End();

    ImGui::PopStyleColor(6); // Pop all theme colors
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
        float button_width = 100; // Width of the Settings button
        float spacing = 10;       // Space from window edges

        // Apply theme colors to the Settings button
        ImGui::PushStyleColor(ImGuiCol_Button, button_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, button_hover_color);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, button_active_color);
        ImGui::PushStyleColor(ImGuiCol_Text, text_color);

        // Position the Settings button in the top-right corner
        ImGui::SetCursorPos(ImVec2(window_width - button_width - spacing, spacing));
        if (ImGui::Button("Settings", ImVec2(button_width, 0)))
        {
            show_settings = !show_settings; // Toggle the settings view visibility
        }
        set_cursor_hand(); // Add hand cursor for settings button

        ImGui::PopStyleColor(4); // Pop theme colors for the button

        // Reset cursor for consistent left alignment of all buttons
        ImGui::SetCursorPos(ImVec2(spacing, spacing));
        show_executable_selector();

        // Move cursor down for next row with minimal spacing
        ImGui::SetCursorPos(ImVec2(spacing, ImGui::GetCursorPosY() + spacing));
        show_iwad_button();

        if (gzdoom_file_dialog.HasSelected())
        {
            std::string new_exec = gzdoom_file_dialog.GetSelected().string();
            // Check if executable is already in the list
            bool exists = false;
            for (const auto &exec_path : config["doom_executables"])
            {
                if (exec_path == new_exec)
                {
                    exists = true;
                    break;
                }
            }
            if (!exists)
            {
                config["doom_executables"].push_back(new_exec);
                // Always select the newly added executable
                config["selected_executable"] = new_exec;
                write_config_file(get_config_file_path(), config);
            }
            gzdoom_file_dialog.ClearSelected();
        }

        if (iwad_file_dialog.HasSelected())
        {
            std::string new_iwad = iwad_file_dialog.GetSelected().string();
            bool found = false;
            for (const auto &iwad : config["iwads"])
            {
                if (iwad.get<std::string>() == new_iwad)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                config["iwads"].push_back(new_iwad);
                config["selected_iwad"] = new_iwad;
                write_config_file(get_config_file_path(), config);
                // Sort the IWAD list alphabetically after adding a new IWAD
                std::sort(config["iwads"].begin(), config["iwads"].end());
            }
            iwad_file_dialog.ClearSelected();
        }

        // Add config file button with same spacing
        ImGui::SetCursorPos(ImVec2(spacing, ImGui::GetCursorPosY() + spacing));
        show_config_button();

        // Add PWAD button with same spacing
        ImGui::SetCursorPos(ImVec2(spacing, ImGui::GetCursorPosY() + spacing));
        show_pwad_button();

        // Keep consistent spacing for remaining elements
        show_pwad_list();
        show_command();
        show_launch_button();
    }
    ImGui::PopStyleColor(2);

    // Show the settings view if the state variable is true
    if (show_settings)
    {
        show_settings_view();
    }
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

void process_dropped_item(const char *dropped_path)
{
    std::filesystem::path path(dropped_path);

    // Helper function to check if file has an allowed extension (WAD, DEH, or EDF)
    auto has_wad_extension = [](const std::string &filename)
    {
        return has_extension(filename, WAD_EXTENSIONS) ||
               has_extension(filename, DEH_EXTENSIONS) ||
               has_extension(filename, EDF_EXTENSIONS);
    };

    if (std::filesystem::exists(path))
    {
        if (std::filesystem::is_directory(path))
        {
            // It's a directory - add it directly to PWAD directories
            bool already_exists = false;
            for (const auto &dir : config["pwad_directories"])
            {
                if (dir == path.string())
                {
                    already_exists = true;
                    break;
                }
            }

            if (!already_exists)
            {
                config["pwad_directories"].push_back(path.string());
                write_config_file(get_config_file_path(), config);
                populate_pwad_list(); // Refresh the PWAD list
            }
        }
        else if (std::filesystem::is_regular_file(path) && has_wad_extension(path.filename().string()))
        {
            // It's a WAD file - add its parent directory
            std::filesystem::path parent_dir = path.parent_path();

            bool already_exists = false;
            for (const auto &dir : config["pwad_directories"])
            {
                if (dir == parent_dir.string())
                {
                    already_exists = true;
                    break;
                }
            }

            if (!already_exists)
            {
                config["pwad_directories"].push_back(parent_dir.string());
                write_config_file(get_config_file_path(), config);
                populate_pwad_list(); // Refresh the PWAD list
            }
        }
    }
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
                // Save current window size before quitting
                int current_width, current_height;
                SDL_GetWindowSize(window, &current_width, &current_height);
                if (validate_window_size(current_width, current_height))
                {
                    config["resolution"] = {current_width, current_height};
                    write_config_file(get_config_file_path(), config);
                }
                done = true;
                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                {
                    // Save current window size before closing
                    int current_width, current_height;
                    SDL_GetWindowSize(window, &current_width, &current_height);
                    if (current_width >= 400 && current_height >= 300 && current_width <= 4096 && current_height <= 4096)
                    {
                        config["resolution"] = {current_width, current_height};
                        write_config_file(get_config_file_path(), config);
                    }
                    done = true;
                }
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    configure_color_buffer();
                    // Update the configuration with the new size
                    int newWidth = event.window.data1;
                    int newHeight = event.window.data2;

                    // Validate the new size before saving
                    if (validate_window_size(newWidth, newHeight))
                    {
                        config["resolution"] = {newWidth, newHeight};
                        // Save will happen on app exit to avoid frequent file writes during resize
                    }
                }
                break;
            case SDL_DROPFILE:
                if (event.drop.file != nullptr)
                {
                    process_dropped_item(event.drop.file);
                    SDL_free(event.drop.file);
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

    // Run migrations after loading config
    config = migrate_config(config);
    write_config_file(config_file_path, config);

    assert(loaded == true);

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

    if (config["config_files"].empty())
    {
        config["config_files"] = nlohmann::json::array();
    }

    if (config["selected_config"].empty())
    {
        config["selected_config"] = "";
    }

    // Initialize doom_executables if it doesn't exist or is null
    if (!config.contains("doom_executables") || config["doom_executables"].is_null())
    {
        config["doom_executables"] = nlohmann::json::array();
    }

    // Initialize selected_executable if it doesn't exist or is null
    if (!config.contains("selected_executable") || config["selected_executable"].is_null())
    {
        config["selected_executable"] = "";
    }

    // Initialize iwads if it doesn't exist or is null
    if (!config.contains("iwads") || config["iwads"].is_null())
    {
        config["iwads"] = nlohmann::json::array();
    }

    // Initialize selected_iwad if it doesn't exist or is null
    if (!config.contains("selected_iwad") || config["selected_iwad"].is_null())
    {
        config["selected_iwad"] = "";
    }

    // Ensure theme field exists with default value
    if (!config.contains("theme") || config["theme"].is_null())
    {
        config["theme"] = "fire";
    }

    // Ensure pin_selected_pwads_to_top field exists with default value
    if (!config.contains("pin_selected_pwads_to_top") || config["pin_selected_pwads_to_top"].is_null())
    {
        config["pin_selected_pwads_to_top"] = true;
    }
    pin_selected_pwads_to_top = config["pin_selected_pwads_to_top"].get<bool>();

    // Ensure group_pwads_by_directory field exists with default value
    if (!config.contains("group_pwads_by_directory") || config["group_pwads_by_directory"].is_null())
    {
        config["group_pwads_by_directory"] = true;
    }
    group_pwads_by_directory = config["group_pwads_by_directory"].get<bool>();

    // Ensure font_scale field exists with default value
    if (!config.contains("font_scale") || config["font_scale"].is_null())
    {
        config["font_scale"] = 1.0f;
    }

    // Ensure sdl_renderer field exists with default value
    if (!config.contains("sdl_renderer") || config["sdl_renderer"].is_null())
    {
        config["sdl_renderer"] = "auto";
    }

    // Ensure sdl_renderer_inherit field exists with default value
    if (!config.contains("sdl_renderer_inherit") || config["sdl_renderer_inherit"].is_null())
    {
        config["sdl_renderer_inherit"] = false;
    }

    // Update the selected_font_scale_index to match the loaded font scale
    static const std::vector<float> font_scales = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f,
                                                   1.0f, 1.1f, 1.2f, 1.3f, 1.4f, 1.5f, 1.6f, 1.7f,
                                                   1.8f, 1.9f, 2.0f};
    float loaded_font_scale = config["font_scale"].get<float>();
    for (int i = 0; i < font_scales.size(); ++i)
    {
        if (font_scales[i] == loaded_font_scale)
        {
            selected_font_scale_index = i;
            break;
        }
    }

    // Save the config after all initializations
    write_config_file(config_file_path, config);

    std::sort(config["iwads"].begin(), config["iwads"].end(),
              [](const std::string &a, const std::string &b)
              {
                  return std::filesystem::path(a).filename() < std::filesystem::path(b).filename();
              });

    std::sort(config["config_files"].begin(), config["config_files"].end(),
              [](const std::string &a, const std::string &b)
              {
                  return std::filesystem::path(a).filename() < std::filesystem::path(b).filename();
              });

    std::sort(config["doom_executables"].begin(), config["doom_executables"].end(),
              [](const std::string &a, const std::string &b)
              {
                  return std::filesystem::path(a).filename() < std::filesystem::path(b).filename();
              });
    // std::sort(config["doom_executables"].begin(), config["doom_executables"].end());
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

    // Setup config first to ensure we have the correct window size
    setup_config_file();

    // Apply SDL renderer hint based on user setting (fix for issue #16)
    std::string renderer_setting = config["sdl_renderer"].get<std::string>();

    if (renderer_setting != "auto")
    {
        SDL_SetHint(SDL_HINT_RENDER_DRIVER, renderer_setting.c_str());

        // Set environment variable if inheritance is enabled
        bool inherit_renderer = config["sdl_renderer_inherit"].get<bool>();
        if (inherit_renderer)
        {
            std::string env_var = "SDL_RENDER_DRIVER=" + renderer_setting;
#ifdef _WIN32
            _putenv(env_var.c_str());
#else
            putenv(const_cast<char *>(env_var.c_str()));
#endif
        }
    }

    // Create window with SDL_Renderer graphics context
    auto window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    std::string title;
    title += "Just Launch Doom v";
    title += VERSION;

    // Validate window size from config
    int window_width = config["resolution"][0];
    int window_height = config["resolution"][1];

    // Apply default sizes for invalid values
    apply_window_size_defaults(window_width, window_height);

    // Ensure maximum reasonable window size based on display
    SDL_DisplayMode display_mode;
    if (SDL_GetCurrentDisplayMode(0, &display_mode) == 0)
    {
        if (window_width > display_mode.w)
            window_width = display_mode.w * 0.9;
        if (window_height > display_mode.h)
            window_height = display_mode.h * 0.9;
    }

    window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              window_width, window_height, window_flags);

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

    // Set font scale from config (loaded earlier in setup_config_file)
    io.FontGlobalScale = config["font_scale"].get<float>();

    // Configure file dialogs with appropriate filters and flags BEFORE using them
    iwad_file_dialog.SetTitle("Select IWAD");
    iwad_file_dialog.SetTypeFilters(WAD_EXTENSIONS);
#ifdef __APPLE__
    iwad_file_dialog.SetPwd("/Applications");
#else
    iwad_file_dialog.SetPwd(std::filesystem::current_path().string());
#endif

    config_file_dialog.SetTitle("Select Config File");
    config_file_dialog.SetTypeFilters(CONFIG_EXTENSIONS);

    pwad_file_dialog.SetTitle("Select PWAD Directory");
    pwad_file_dialog.SetTypeFilters(WAD_EXTENSIONS);

    gzdoom_file_dialog.SetTitle("Select Doom Executable");
    gzdoom_file_dialog.SetTypeFilters(EXECUTABLE_EXTENSIONS);

    // Now populate lists (config was already set up earlier)
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

int main(int argc, char **argv)
{
    setup();
    update();
    clean_up();

    return 0;
}