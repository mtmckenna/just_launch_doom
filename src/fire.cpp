//
// Created by Matt McKenna on 3/19/24.
//

#include <SDL.h>

#include "fire.h"

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
