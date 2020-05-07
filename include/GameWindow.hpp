#ifndef GAMEWINDOW_HPP
#define GAMEWINDOW_HPP

#include "NES.hpp"
#include <SDL2/SDL.h>
#include <memory>

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;
const int CHANNELS = 3;
const int SCREEN_FPS = 60;
const int SCREEN_TICKS_PER_FRAME = 1000 / SCREEN_FPS;

class GameWindow
{
public:
    GameWindow();
    void runNES();
    void updateScreen();
    ~GameWindow();
private:
    std::unique_ptr<NES> nes;
    SDL_Window* window = nullptr;
    SDL_Surface* screenSurface = nullptr;
    SDL_Surface* windowSurface = nullptr;
    std::shared_ptr<char[]> frameBuffer;
};

#endif