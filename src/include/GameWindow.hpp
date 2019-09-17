#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <SDL2/SDL.h>
#include "NES.hpp"

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int channels = 3;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

class GameWindow
{
public:
    GameWindow();
    void run();
    void update();
    ~GameWindow();
private:
    NES* nes;
    SDL_Window* window = nullptr;
    SDL_Surface* screenSurface = nullptr;
    char* frameBuffer;
};

#endif