#include <iostream>
#include <cassert>
#include <fstream>
#include <SDL.h>
#include <filesystem>

#include "nlohmann/json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imfilebrowser.h"

#include "fire.h"

#if !SDL_VERSION_ATLEAST(2, 0, 17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

int renderer_width, renderer_height;
int color_buffer_width, color_buffer_height;
int launch_button_height = 35;
char command_buf[1024] = "THIS IS THE COMMAND";
//std::vector<bool> pwads(120, false);
std::vector<std::pair<std::string, bool>> pwads;

uint32_t *color_buffer = nullptr;
nlohmann::json config = {
        {"gzdoom_filepath", ""},
        {"resolution",  {800, 600}},
        {"pwad_path",   ""},
        {"iwad_filepath",   ""},
        {"selected_pwads", nlohmann::json::array()}
};

const std::string APP_NAME = "just_launch_doom";
ImGui::FileBrowser gzdoom_file_dialog;
ImGui::FileBrowser iwad_file_dialog;
ImGui::FileBrowser pwad_file_dialog(ImGuiFileBrowserFlags_SelectDirectory);
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
SDL_Texture *color_buffer_texture;
SDL_Window *window;
SDL_Renderer *renderer;

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

void set_cursor_hand()
{
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }
}

std::string get_application_support_path()
{
    const char *homeDir = getenv("HOME");
    assert(homeDir != nullptr);

    std::string path = std::string(homeDir) + "/Library/Application Support/" + APP_NAME;
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
//        std::string pwad = " -file " + config["pwad_path"];
    std::string pwad = "";

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
    } else
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
    } else
    {
        write_config_file(path, config);
    }

    return true;
}


void populate_pwad_list() {
    // Clear the existing vector
    pwads.clear();

    std::string pwadPath = config["pwad_path"];

    // Check if the directory exists and is accessible
    if (!std::filesystem::exists(pwadPath) || !std::filesystem::is_directory(pwadPath)) {
        // Handle the error appropriately
        std::cerr << "The specified PWAD path does not exist or is not a directory." << std::endl;
        return;
    }

    // Iterate over the directory
    for (const auto& entry : std::filesystem::directory_iterator(pwadPath)) {
        // Check if it's a regular file
        if (entry.is_regular_file()) {
            // You can add more checks here for file extension if necessary
            std::string filePath = entry.path().string();

            // Add the file path and set the selection flag to false
            pwads.emplace_back(filePath, false);
        }
    }
}

void show_pwad_list()
{
    ImGui::SeparatorText("Select PWAD(s)");

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 listSize = ImVec2(avail.x, avail.y - 2 * ImGui::GetFrameHeightWithSpacing() - launch_button_height); // Reserve space for one line height for the next widget, if necessary

    if (ImGui::BeginListBox("##pwad_list_id", listSize))
    {
        ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.0f, 0.5f, 0.0f, 1.0f)); // Green checkmark
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1.0f, 1.0f, 1.0f, 0.1f)); // Semi-transparent white background

        for (size_t i = 0; i < pwads.size(); i++)
        {
            // Using PushID is important for ensuring unique IDs within a loop
            ImGui::PushID(i);

            // Checkbox for selection. Use the label from the pair
            if (ImGui::Checkbox(pwads[i].first.c_str(), &pwads[i].second))
            {
                // Here you can handle the change in selection if needed.
            }

            ImGui::PopID(); // Don't forget to pop the ID after each element
        }

        ImGui::PopStyleColor(2); // Pop both colors at once
        ImGui::EndListBox();
    }
}

void show_launch_button()
{
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f,0.5,0.0, .75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f, 0.0f, 0.0f, .75f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.0f, 0.0f, .75f)); // Darker

    if (ImGui::Button("Just Launch Doom!", ImVec2(-1, launch_button_height)))
    {
        std::string cmd = get_launch_command();
        std::cout << cmd << std::endl;
        config["cmd"] = cmd;
        system(cmd.c_str());
    }
    ImGui::PopStyleColor(3);
    set_cursor_hand();
}

void show_gzdoom_button()
{
    // Show button
    if (ImGui::Button("Select GZDoom executable"))
    {
        gzdoom_file_dialog.SetTitle("Select GZDoom executable");

        std::string gzdoom_filepath = config["gzdoom_filepath"];

        if (gzdoom_filepath.empty())
        {
            gzdoom_file_dialog.SetPwd(get_initial_application_path());
        } else
        {
            gzdoom_file_dialog.SetPwd(gzdoom_filepath);
        }

        gzdoom_file_dialog.Open();
    }
    help_marker("e.g. /Applications/GZDoom.app/Contents/MacOS/gzdoom");
    set_cursor_hand();

    // Update path in settings
    gzdoom_file_dialog.Display();
    if (gzdoom_file_dialog.HasSelected())
    {
        config["gzdoom_filepath"] = gzdoom_file_dialog.GetSelected().string();
        gzdoom_file_dialog.ClearSelected();
    }

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["gzdoom_filepath"];

    if (path.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No GZDoom executable selected");
    } else
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", path.c_str());
    }
}

void show_iwad_button()
{
    // Show button

    std::string filepath_string = config["iwad_filepath"];

    if (ImGui::Button("Select IWAD"))
    {
        iwad_file_dialog.SetTitle("Select IWAD");

        if (filepath_string.empty())
        {
            iwad_file_dialog.SetPwd("~/");
        } else
        {
            std::filesystem::path file_path(filepath_string);
            std::filesystem::path directoryPath = file_path.parent_path();
            iwad_file_dialog.SetPwd(directoryPath.c_str());
        }

        iwad_file_dialog.Open();
    }
    help_marker("e.g. doom.wad, doom2.wad, heretic.wad, hexen.wad, strife1.wad, etc.");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    // Update path in settings
    iwad_file_dialog.Display();
    if (iwad_file_dialog.HasSelected())
    {
        config["iwad_filepath"] = iwad_file_dialog.GetSelected().string();
        iwad_file_dialog.ClearSelected();
    }

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["iwad_filepath"];

    if (path.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No IWAD selected (e.g. doom.wad)");
    } else
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", path.c_str());
    }
}

void show_pwad_button()
{
    // Show button

    if (ImGui::Button("Select PWAD folder"))
    {
        pwad_file_dialog.SetTitle("Select IWAD");

        if (config["pwad_path"].empty())
        {
            pwad_file_dialog.SetPwd("~/");
        } else
        {
            pwad_file_dialog.SetPwd(config["pwad_path"]);
        }

        pwad_file_dialog.Open();
    }
    help_marker("e.g. ~/doom_wads/");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    // Update path in settings
    pwad_file_dialog.Display();
    if (pwad_file_dialog.HasSelected())
    {
        config["pwad_path"] = pwad_file_dialog.GetSelected().string();
        std::cout << pwad_file_dialog.GetSelected().string() << std::endl;
        pwad_file_dialog.ClearSelected();
    }

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["pwad_path"];

    if (path.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No PWAD folder selected");
    } else
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", path.c_str());
    }
}

void show_command()
{
    snprintf(command_buf, IM_ARRAYSIZE(command_buf), "%s", get_launch_command().c_str());
    ImGui::SeparatorText("Command to run");
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.75f));
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    if (ImGui::InputText("##command_id", command_buf, IM_ARRAYSIZE(command_buf)))
    {
        // This code gets executed if the input text changes.
        // Here you can handle the updated input, for example:
        std::string text = command_buf; // Now you have the text in an std::string if you need to use it elsewhere
    }

    ImGui::PopStyleColor();
}

void show_ui()
{
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 50));
    // Put everything in a full screen window
    if (ImGui::Begin("fullscreen window", nullptr, window_flags))
    {
        show_gzdoom_button();
        show_iwad_button();
        show_pwad_button();
        show_pwad_list();
        show_command();
        show_launch_button();
    }
    ImGui::PopStyleColor();
}

void set_color_buffer_size()
{
    const int MAX_WIDTH = 640;
    const int MAX_HEIGHT = 480;
    float aspect_ratio_x = (float) renderer_width / (float) renderer_height;
    float aspect_ratio_y = (float) renderer_height / (float) renderer_width;

    if (aspect_ratio_x > aspect_ratio_y)
    {
        color_buffer_width = MAX_WIDTH;
        color_buffer_height = MAX_WIDTH * aspect_ratio_y;
    } else
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
        SDL_SetRenderDrawColor(renderer, (Uint8) (clear_color.x * 255), (Uint8) (clear_color.y * 255),
                               (Uint8) (clear_color.z * 255), (Uint8) (clear_color.w * 255));
        SDL_RenderClear(renderer);

        draw_fire(color_buffer_texture, color_buffer, color_buffer_width, color_buffer_height);
        SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);

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
}

int setup()
{

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
    auto window_flags = (SDL_WindowFlags) (SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    window = SDL_CreateWindow("Just Launch Doom!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    io.FontGlobalScale = 1.0;

    setup_config_file();
    populate_pwad_list();

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
