//
// Created by Matt McKenna on 3/19/24.
//

#include <SDL.h>
#include <cstdint>
#include <vector>

#include "fire.h"

std::vector<int> fire_pixels;
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

void spread_fire(SDL_Texture* texture, uint32_t *colorBuffer, int windowWidth, int from)
{
    int pixelValue = fire_pixels[from];
    int to = from - windowWidth;

    if (pixelValue != 0)
    {
        int rand = std::rand() % 4;
        to = from - windowWidth - rand + 1;
        pixelValue =  fire_pixels[from] - (rand & 1);
    }

    fire_pixels[to] = pixelValue;
}


void draw_fire(SDL_Texture* texture, uint32_t *colorBuffer, int windowWidth, int window_height)
{
    const int fire_height = 500;

    if (fire_pixels.empty())
    {
        fire_pixels.resize(windowWidth * window_height);
        for (int i = 0; i < windowWidth * window_height; i++)
        {
            if (i > windowWidth * (window_height - 1) - windowWidth)
            {
                fire_pixels[i] = COLOR_SIZE - 1;
            }
            else
            {
                fire_pixels[i] = 0;
            }
        }
    }

    for (int x = 0; x < windowWidth; x++)
    {
        for (int y = window_height - 1; y > window_height - fire_height; y--)
        {
            const int index = x + (windowWidth * y);
            spread_fire(texture, colorBuffer, windowWidth, index);
            colorBuffer[index] = COLORS[fire_pixels[index]];
//            colorBuffer[index] = 0x110000FF;
        }
    }

    SDL_UpdateTexture(texture, nullptr, colorBuffer, (int) (windowWidth * sizeof(uint32_t)));
}

void drawSquareInTexture(SDL_Texture* texture, uint32_t *colorBuffer, int squareSize, Uint32 squareColor, int windowWidth) {

    // Draw a square at the top-left corner
    for (int y = 0; y < squareSize; ++y) {
        for (int x = 0; x < squareSize; ++x) {
            colorBuffer[y * windowWidth + x] = squareColor;
        }
    }

    SDL_UpdateTexture(texture, nullptr, colorBuffer, (int) (windowWidth * sizeof(uint32_t)));
}

void clearTexture(SDL_Texture* texture, uint32_t* colorBuffer, int screenWidth, int screenHeight, Uint32 clearColor) {
    // Fill the entire texture with the clear color
    for (int y = 0; y < screenHeight; ++y) {
        for (int x = 0; x < screenWidth; ++x) {
            colorBuffer[y * screenWidth + x] = clearColor;
        }
    }

    SDL_UpdateTexture(texture, nullptr, colorBuffer, (int) (screenWidth * sizeof(uint32_t)));
}
