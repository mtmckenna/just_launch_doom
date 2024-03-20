//
// Created by Matt McKenna on 3/19/24.
//

#ifndef SDL_IMGUI_FIRE_H
#define SDL_IMGUI_FIRE_H


void drawSquareInTexture(SDL_Texture* texture, uint32_t* colorBuffer, int squareSize, Uint32 squareColor, int windowWidth);
void clearTexture(SDL_Texture* texture, uint32_t* colorBuffer, int screenWidth, int screenHeight, Uint32 clearColor);
void draw_fire(SDL_Texture* texture, uint32_t *colorBuffer, int windowWidth, int window_height);


#endif //SDL_IMGUI_FIRE_H
