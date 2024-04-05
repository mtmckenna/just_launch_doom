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
uint32_t *color_buffer = nullptr;
nlohmann::json config = {
        {"gzdoom_path", ""},
        {"resolution",  {800, 600}},
        {"pwad_path",   ""},
        {"iwad_path",   ""}
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
    std::string command = config["gzdoom_path"];
    std::string iwad = config["iwad_path"];
//        std::string pwad = " -file " + config["pwad_path"];
    std::string cmd = "\"" + command + "\"" + " " + "-iwad " + "\"" + iwad + "\""; //+ pwad;
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

void show_pwad_list()
{
    ImGui::SeparatorText("Select PWAD(s)");

    std::vector<std::string> items;
    for (int i = 0; i < 120; i++)
    {
        items.push_back("Item " + std::to_string(i));
    }

    static int selectedItem = -1; // Index of the selected item, -1 means no selection

    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 listSize = ImVec2(avail.x, avail.y - 3 * ImGui::GetFrameHeightWithSpacing() - launch_button_height); // Reserve space for one line height for the next widget, if necessary

    // Create a list box with a given size that fills the vertical space
    if (ImGui::BeginListBox("##pwad_list_id", listSize))
    {
        for (int i = 0; i < items.size(); i++)
        {
            const bool isSelected = (selectedItem == i);
            if (ImGui::Selectable(items[i].c_str(), isSelected))
            {
                selectedItem = i;
                // If an item is selected (user clicked on an item)
                std::cout << "Item " << i << " clicked!" << std::endl;
            }

            // Set the initial focus when opening the list box
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }

}

void show_launch_button()
{
    // add some vertical padding
    ImGui::Dummy(ImVec2(0.0f, 10.0f));
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f,0.0,0.0, 1.0f));

    if (ImGui::Button("Just Launch Doom!", ImVec2(-1, launch_button_height)))
    {
        std::string cmd = get_launch_command();
        std::cout << cmd << std::endl;
        system(cmd.c_str());
    }
    ImGui::PopStyleColor();
    set_cursor_hand();
}

void show_gzdoom_button()
{
    // Show button
    if (ImGui::Button("Select GZDoom executable"))
    {
        gzdoom_file_dialog.SetTitle("Select GZDoom executable");

        if (config["gzdoom_path"].empty())
        {
            gzdoom_file_dialog.SetPwd(get_initial_application_path());
        } else
        {
            gzdoom_file_dialog.SetPwd(config["gzdoom_path"]);
        }

        gzdoom_file_dialog.Open();
    }
    help_marker("e.g. /Applications/GZDoom.app/Contents/MacOS/gzdoom");
    set_cursor_hand();

    // Update path in settings
    gzdoom_file_dialog.Display();
    if (gzdoom_file_dialog.HasSelected())
    {
        config["gzdoom_path"] = gzdoom_file_dialog.GetSelected().string();
        gzdoom_file_dialog.ClearSelected();
    }

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["gzdoom_path"];

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

    if (ImGui::Button("Select IWAD"))
    {
        iwad_file_dialog.SetTitle("Select IWAD");

        if (config["pwad_path"].empty())
        {
            iwad_file_dialog.SetPwd("~/");
        } else
        {
            std::filesystem::path filePath(config["iwad_path"]);
            std::filesystem::path directoryPath = filePath.parent_path();
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
        config["iwad_path"] = iwad_file_dialog.GetSelected().string();
        std::cout << iwad_file_dialog.GetSelected().string() << std::endl;
        iwad_file_dialog.ClearSelected();
    }

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["iwad_path"];

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
//    std::string gzdoom_path = config["gzdoom_path"].get<std::string>();
    snprintf(command_buf, IM_ARRAYSIZE(command_buf), "%s", get_launch_command().c_str());
    ImGui::SeparatorText("Command to run Doom");
    if (ImGui::InputText("##command_id", command_buf, IM_ARRAYSIZE(command_buf)))
    {
        // This code gets executed if the input text changes.
        // Here you can handle the updated input, for example:
        std::string text = command_buf; // Now you have the text in an std::string if you need to use it elsewhere
    }

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

    if (config["gzdoom_path"].empty())
    {
        config["gzdoom_path"] = "";
    }

    if (config["pwad_path"].empty())
    {
        config["pwad_path"] = "";
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

    window = SDL_CreateWindow("Launch Doom!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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

//    set_color_buffer_size();
//
//    color_buffer = new uint32_t[sizeof(uint32_t) * color_buffer_width * color_buffer_height];
//    color_buffer_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
//                                             color_buffer_width, color_buffer_height);

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
