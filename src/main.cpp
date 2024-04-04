#include <iostream>
#include <cassert>
#include <fstream>
#include <SDL.h>

#include "nlohmann/json.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imfilebrowser.h"

#include "fire.h"


#if !SDL_VERSION_ATLEAST(2,0,17)
#error This backend requires SDL 2.0.17+ because of SDL_RenderGeometry() function
#endif

int rendererWidth, rendererHeight;
uint32_t *colorBuffer = nullptr;
nlohmann::json config = {
    {"gzdoom_path", ""},
    {"resolution", {800, 600}},
    {"pwad_path", ""}
};

const std::string APP_NAME = "just_launch_doom";
ImGui::FileBrowser fileDialog;
ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
SDL_Texture* colorBufferTexture;
SDL_Window* window;
SDL_Renderer* renderer;

std::string get_application_support_path()
{
    const char* homeDir = getenv("HOME");
    assert(homeDir != nullptr);

    std::string path = std::string(homeDir) + "/Library/Application Support/" + APP_NAME;
    std::filesystem::create_directories(path);
    return path;
}

std::string get_initial_application_path()
{
    return "/Applications";
}

std::string get_config_file_path() {
    return get_application_support_path() + "/config.json";
}

bool write_config_file(const std::string& path, nlohmann::json& config)
{
    // Write to file
    std::ofstream file(path);
    if (file.is_open()) {
        file << config.dump(4); // dump with 4 spaces indentation
        file.close();
    } else {
        return false;
    }

    return true;
}

bool load_config_file(std::string& path, nlohmann::json& config)
{
    // if file exists, load it and put put data into config
    // if not, create it and write some default settings
    std::ifstream file(path);
    if (file.is_open()) {
        file >> config;
        file.close();
    } else {
        write_config_file(path, config);
    }

    return true;
}

void show_iwad_list()
{
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(0,0,0,50));

    std::vector<std::string> items;
    for (int i = 0; i < 120; i++) { // Dynamically add items
        items.push_back("Item " + std::to_string(i));
    }

    static int selectedItem = -1; // Index of the selected item, -1 means no selection

    // Set the list box size to use all available space
    ImVec2 listBoxSize = ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y); // -FLT_MIN makes width stretch to the available space

    // Create a list box with a given size that fills the vertical space
    if (ImGui::BeginListBox("##MyDynamicListBox", listBoxSize)) {

        for (int i = 0; i < items.size(); i++) {
            const bool isSelected = (selectedItem == i);
            if (ImGui::Selectable(items[i].c_str(), isSelected)) {
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

    ImGui::PopStyleColor();
}

void show_gzdoom_button()
{
    // Show button
    if(ImGui::Button("Select GZDoom executable"))
    {
        fileDialog.SetTitle("Select GZDoom executable");

        if (config["gzdoom_path"].empty())
        {
            fileDialog.SetPwd(get_initial_application_path());
        }
        else
        {
            fileDialog.SetPwd(config["gzdoom_path"]);
        }

        fileDialog.Open();
    }

    // Update path in settings
    fileDialog.Display();
    if(fileDialog.HasSelected())
    {
        config["gzdoom_path"] = fileDialog.GetSelected().string();
        fileDialog.ClearSelected();
    }

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["gzdoom_path"];

    if (path.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No GZDoom executable selected");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", path.c_str());
    }
}

void show_pwad_button()
{
    // Show button
    if(ImGui::Button("Select PWAD"))
    {
        fileDialog.SetTitle("Select PWAD");

        if (config["pwad_path"].empty())
        {
            fileDialog.SetPwd("~/");
        }
        else
        {
            fileDialog.SetPwd(config["pwad_path"]);
        }

        fileDialog.Open();
    }

    // Update path in settings
    fileDialog.Display();
    if(fileDialog.HasSelected())
    {
        config["pwad_path"] = fileDialog.GetSelected().string();
        fileDialog.ClearSelected();
    }

    // Display path in UI
    ImGui::SameLine();
    std::string path = config["pwad_path"];

    if (path.empty())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "No PWAD selected (e.g. doom.wad)");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", path.c_str());
    }

}


void update()
{
    ImGuiIO& io = ImGui::GetIO();
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
                        // Update the configuration with the new size
                        int newWidth = event.window.data1;
                        int newHeight = event.window.data2;
                        config["resolution"] = { newWidth, newHeight };
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
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;


        // Put everything in a full screen window
        if(ImGui::Begin("fullscreen window", nullptr, window_flags))
        {
            show_gzdoom_button();
            show_pwad_button();
            show_iwad_list();

        }

        ImGui::End();

        // Rendering
        ImGui::Render();
        SDL_RenderSetScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColor(renderer, (Uint8)(clear_color.x * 255), (Uint8)(clear_color.y * 255), (Uint8)(clear_color.z * 255), (Uint8)(clear_color.w * 255));
        SDL_RenderClear(renderer);

        draw_fire(colorBufferTexture, colorBuffer, rendererWidth, rendererHeight);
        SDL_RenderCopy(renderer, colorBufferTexture, NULL, NULL);

        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }
}

void setup_config_file()
{
    std::string config_file_path = get_config_file_path();
    bool loaded = load_config_file(config_file_path, config);

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

    assert(loaded == true);
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
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    window = SDL_CreateWindow("Just Launch Doom", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
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

    SDL_GetRendererOutputSize(renderer, &rendererWidth, &rendererHeight);
    colorBuffer = new uint32_t[sizeof(uint32_t) * rendererWidth * rendererHeight];

    colorBufferTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, rendererWidth, rendererHeight);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    io.FontGlobalScale = 1.5;

    setup_config_file();
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

int main(int, char**)
{
    setup();
    update();
    clean_up();

    return 0;
}
