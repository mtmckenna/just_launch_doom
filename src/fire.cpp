#include <SDL.h>
#include <vector>
#include <random>

#include "fire.h"

std::vector<int> fire_pixels;
std::uniform_int_distribution<int> distrib(0, 3);
std::random_device rd;
std::default_random_engine gen(rd());

const uint32_t COLORS[] = {
        0xFF070707, 0xFF1F0707, 0xFF2F0F07, 0xFF470F07, 0xFF571707,
        0xFF671F07, 0xFF771F07, 0xFF8F2707, 0xFF9F2F07, 0xFFAF3F07,
        0xFFBF4707, 0xFFC74707, 0xFFDF4F07, 0xFFDF5707, 0xFFDF5707,
        0xFFD75F07, 0xFFD75F07, 0xFFD7670F, 0xFFCF6F0F, 0xFFCF770F,
        0xFFCF7F0F, 0xFFCF8717, 0xFFC78717, 0xFFC78F17, 0xFFC7971F,
        0xFFBF9F1F, 0xFFBF9F1F, 0xFFBFA727, 0xFFBFA727, 0xFFBFAF2F,
        0xFFB7AF2F, 0xFFB7B72F, 0xFFB7B737, 0xFFCFCF6F, 0xFFDFDF9F,
        0xFFEFEFC7, 0xFFFFFFFF
};

const int COLOR_SIZE = sizeof(COLORS) / sizeof(uint32_t);

// https://github.com/fabiensanglard/DoomFirePSX/blob/master/flames.html
void spread_fire(int window_width, int from)
{
    int pixelValue = fire_pixels[from];
    if (pixelValue == 0)
    {
        fire_pixels[from - window_width] = 0;
    }
    else
    {
        int rand = distrib(gen);
        int to = from - rand + 1;
        fire_pixels[to - window_width] = pixelValue - (rand & 1);
    }
}

void draw_fire(SDL_Texture* texture, uint32_t *color_buffer, int window_width, int window_height)
{

    if (fire_pixels.empty())
    {
        fire_pixels.resize(window_width * window_height, 0 );
        for (int i = 0; i < window_width * window_height; i++)
        {
            if (i > window_width * (window_height - 1) - window_width)
            {
                fire_pixels[i] = COLOR_SIZE - 1;
            }

        }
    }

    for (int x = 0; x < window_width; x++)
    {
        for (int y = window_height - 1; y > 0; y--)
        {
            const int index = x + (window_width * y);
            spread_fire(window_width, index);
            color_buffer[index] = COLORS[fire_pixels[index]];
        }
    }

    SDL_UpdateTexture(texture, nullptr, color_buffer, (int) (window_width * sizeof(uint32_t)));
}